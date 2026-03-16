/**************************************************************************/
/*  mcp_tools_linter.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "mcp_tools_linter.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/string/ustring.h"

// ---------------------------------------------------------------------------
// Diagnostic helper
// ---------------------------------------------------------------------------

static Dictionary _make_diagnostic(const String &p_rule, const String &p_severity, int p_line, int p_column, const String &p_message, const String &p_suggestion = "", bool p_auto_fixable = false) {
	Dictionary d;
	d["rule"] = p_rule;
	d["severity"] = p_severity;
	d["line"] = p_line;
	d["column"] = p_column;
	d["message"] = p_message;
	d["suggestion"] = p_suggestion;
	d["auto_fixable"] = p_auto_fixable;
	return d;
}

// ---------------------------------------------------------------------------
// Lint analysis helpers
// ---------------------------------------------------------------------------

static bool _is_snake_case(const String &p_name) {
	for (int i = 0; i < p_name.length(); i++) {
		char32_t c = p_name[i];
		if (c >= 'A' && c <= 'Z') {
			return false;
		}
	}
	return true;
}

static void _collect_gd_files_recursive(const String &p_dir, Vector<String> &r_files) {
	Ref<DirAccess> dir = DirAccess::open(p_dir);
	if (dir.is_null()) {
		return;
	}

	dir->list_dir_begin();
	String file_name = dir->get_next();
	while (!file_name.is_empty()) {
		String full_path = p_dir.path_join(file_name);
		if (dir->current_is_dir()) {
			if (file_name != "." && file_name != ".." && !file_name.begins_with(".")) {
				_collect_gd_files_recursive(full_path, r_files);
			}
		} else if (file_name.get_extension() == "gd") {
			r_files.push_back(full_path);
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();
}

static Array _lint_source(const String &p_source, const String &p_path) {
	Array diagnostics;
	PackedStringArray lines = p_source.split("\n");

	// Track function state for multi-line analysis.
	int current_func_start = -1;
	String current_func_name;
	int branch_count = 0;

	// Track declared variables for unused detection.
	struct VarDecl {
		String name;
		int line;
	};
	Vector<VarDecl> declared_vars;

	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i];
		String stripped = line.strip_edges();
		int line_num = i + 1;

		// --- UNTYPED_DECLARATION ---
		// Match: var name = value (no : for type annotation).
		if (stripped.begins_with("var ") || stripped.begins_with("@export var ")) {
			String after_var = stripped.substr(stripped.find("var ") + 4).strip_edges();
			// Check if there is a colon for type annotation before '='.
			int eq_pos = after_var.find("=");
			int colon_pos = after_var.find(":");
			if (colon_pos < 0 || (eq_pos >= 0 && colon_pos > eq_pos)) {
				String var_name = after_var;
				if (eq_pos >= 0) {
					var_name = after_var.substr(0, eq_pos).strip_edges();
				}
				diagnostics.push_back(_make_diagnostic(
						"UNTYPED_DECLARATION", "warning", line_num, 1,
						"Variable '" + var_name + "' has no type annotation",
						"Add type annotation: var " + var_name + ": Type = ...",
						true));
			}

			// Track declared variable for UNUSED_VARIABLE check.
			String var_name = after_var;
			int eq = after_var.find("=");
			int col = after_var.find(":");
			if (eq >= 0 && (col < 0 || eq < col)) {
				var_name = after_var.substr(0, eq).strip_edges();
			} else if (col >= 0) {
				var_name = after_var.substr(0, col).strip_edges();
			}
			if (!var_name.is_empty()) {
				VarDecl vd;
				vd.name = var_name;
				vd.line = line_num;
				declared_vars.push_back(vd);
			}
		}

		// --- MISSING_RETURN_TYPE ---
		if (stripped.begins_with("func ")) {
			// Check for function end state of previous function.
			if (current_func_start >= 0) {
				int func_length = line_num - current_func_start;
				if (func_length > 50) {
					diagnostics.push_back(_make_diagnostic(
							"LONG_FUNCTION", "warning", current_func_start, 1,
							"Function '" + current_func_name + "' is " + itos(func_length) + " lines long (limit: 50)",
							"Consider breaking this function into smaller functions"));
				}
				if (branch_count > 10) {
					diagnostics.push_back(_make_diagnostic(
							"CYCLOMATIC_COMPLEXITY", "warning", current_func_start, 1,
							"Function '" + current_func_name + "' has high cyclomatic complexity (" + itos(branch_count) + " branches)",
							"Consider simplifying the control flow"));
				}
			}

			String func_decl = stripped.substr(5);
			int paren_close = func_decl.find(")");
			if (paren_close >= 0) {
				String after_paren = func_decl.substr(paren_close + 1).strip_edges();
				if (!after_paren.begins_with("->")) {
					String fname = func_decl.substr(0, func_decl.find("(")).strip_edges();
					diagnostics.push_back(_make_diagnostic(
							"MISSING_RETURN_TYPE", "warning", line_num, 1,
							"Function '" + fname + "' has no return type annotation",
							"Add return type: func " + fname + "(...) -> ReturnType:",
							true));
				}
			}

			// Track function name and start for LONG_FUNCTION / CYCLOMATIC_COMPLEXITY.
			String fname = func_decl.substr(0, func_decl.find("(")).strip_edges();
			current_func_start = line_num;
			current_func_name = fname;
			branch_count = 0;

			// --- MISSING_DOCSTRING ---
			// Check if previous non-empty line is a ## doc comment.
			if (!fname.begins_with("_")) {
				bool has_doc = false;
				for (int j = i - 1; j >= 0; j--) {
					String prev = lines[j].strip_edges();
					if (prev.is_empty()) {
						continue;
					}
					if (prev.begins_with("##")) {
						has_doc = true;
					}
					break;
				}
				if (!has_doc) {
					diagnostics.push_back(_make_diagnostic(
							"MISSING_DOCSTRING", "info", line_num, 1,
							"Public function '" + fname + "' has no documentation comment",
							"Add a ## doc comment above the function"));
				}
			}

			// Check parameter type annotations.
			int paren_open = func_decl.find("(");
			if (paren_open >= 0 && paren_close > paren_open + 1) {
				String params_str = func_decl.substr(paren_open + 1, paren_close - paren_open - 1);
				PackedStringArray params = params_str.split(",");
				for (int p = 0; p < params.size(); p++) {
					String param = params[p].strip_edges();
					if (!param.is_empty() && param.find(":") < 0) {
						diagnostics.push_back(_make_diagnostic(
								"UNTYPED_DECLARATION", "warning", line_num, 1,
								"Parameter '" + param + "' has no type annotation",
								"Add type: " + param + ": Type",
								true));
					}
				}
			}
		}

		// --- SIGNAL_NAMING ---
		if (stripped.begins_with("signal ")) {
			String signal_name = stripped.substr(7).strip_edges();
			int paren = signal_name.find("(");
			if (paren >= 0) {
				signal_name = signal_name.substr(0, paren).strip_edges();
			}
			if (!_is_snake_case(signal_name)) {
				diagnostics.push_back(_make_diagnostic(
						"SIGNAL_NAMING", "warning", line_num, 1,
						"Signal '" + signal_name + "' is not in snake_case",
						"Rename signal to use snake_case",
						true));
			}
		}

		// --- NODE_PATH_LITERAL ---
		// Detect NodePath("...") or $"..." or $ followed by a path.
		if (stripped.find("NodePath(\"") >= 0 || stripped.find("get_node(\"") >= 0) {
			diagnostics.push_back(_make_diagnostic(
					"NODE_PATH_LITERAL", "info", line_num, 1,
					"Hardcoded NodePath string detected",
					"Consider using @export var or %UniqueNode syntax instead"));
		}

		// --- CYCLOMATIC_COMPLEXITY tracking ---
		if (stripped.begins_with("if ") || stripped.begins_with("elif ") ||
				stripped.begins_with("match ") || stripped.begins_with("while ") ||
				stripped.begins_with("for ")) {
			branch_count++;
		}
	}

	// Handle last function.
	if (current_func_start >= 0) {
		int func_length = lines.size() - current_func_start + 1;
		if (func_length > 50) {
			diagnostics.push_back(_make_diagnostic(
					"LONG_FUNCTION", "warning", current_func_start, 1,
					"Function '" + current_func_name + "' is " + itos(func_length) + " lines long (limit: 50)",
					"Consider breaking this function into smaller functions"));
		}
		if (branch_count > 10) {
			diagnostics.push_back(_make_diagnostic(
					"CYCLOMATIC_COMPLEXITY", "warning", current_func_start, 1,
					"Function '" + current_func_name + "' has high cyclomatic complexity (" + itos(branch_count) + " branches)",
					"Consider simplifying the control flow"));
		}
	}

	// --- UNUSED_VARIABLE ---
	// Basic: check if the variable name appears elsewhere in the source after declaration.
	for (const VarDecl &vd : declared_vars) {
		int occurrences = 0;
		for (int i = 0; i < lines.size(); i++) {
			if (i + 1 == vd.line) {
				continue; // Skip the declaration line itself.
			}
			String line = lines[i];
			// Simple substring search for the variable name as a whole word.
			int search_pos = 0;
			while (search_pos < line.length()) {
				int found = line.find(vd.name, search_pos);
				if (found < 0) {
					break;
				}
				// Check word boundaries.
				bool left_ok = (found == 0) || !is_ascii_identifier_char(line[found - 1]);
				bool right_ok = (found + vd.name.length() >= line.length()) || !is_ascii_identifier_char(line[found + vd.name.length()]);
				if (left_ok && right_ok) {
					occurrences++;
					break;
				}
				search_pos = found + 1;
			}
			if (occurrences > 0) {
				break;
			}
		}
		if (occurrences == 0) {
			diagnostics.push_back(_make_diagnostic(
					"UNUSED_VARIABLE", "warning", vd.line, 1,
					"Variable '" + vd.name + "' is declared but never used",
					"Remove the unused variable or prefix with _ to indicate intentional non-use",
					true));
		}
	}

	return diagnostics;
}

// ---------------------------------------------------------------------------
// lint_script
// ---------------------------------------------------------------------------

static Dictionary _tool_lint_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	if (file.is_null()) {
		return mcp_error("Cannot open file: " + path);
	}

	String source = file->get_as_text();
	file->close();

	Array diagnostics = _lint_source(source, path);

	Dictionary result = mcp_success();
	result["path"] = path;
	result["diagnostics"] = diagnostics;
	result["diagnostic_count"] = diagnostics.size();
	return result;
}

// ---------------------------------------------------------------------------
// lint_project
// ---------------------------------------------------------------------------

static Dictionary _tool_lint_project(const Dictionary &p_args) {
	Vector<String> gd_files;
	_collect_gd_files_recursive("res://", gd_files);

	Array file_results;
	int total_diagnostics = 0;

	for (const String &file_path : gd_files) {
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		if (file.is_null()) {
			continue;
		}

		String source = file->get_as_text();
		file->close();

		Array diagnostics = _lint_source(source, file_path);
		if (!diagnostics.is_empty()) {
			Dictionary file_result;
			file_result["path"] = file_path;
			file_result["diagnostics"] = diagnostics;
			file_result["diagnostic_count"] = diagnostics.size();
			file_results.push_back(file_result);
			total_diagnostics += diagnostics.size();
		}
	}

	Dictionary result = mcp_success();
	result["files"] = file_results;
	result["files_with_issues"] = file_results.size();
	result["total_files_scanned"] = gd_files.size();
	result["total_diagnostics"] = total_diagnostics;
	return result;
}

// ---------------------------------------------------------------------------
// get_lint_rules
// ---------------------------------------------------------------------------

static Dictionary _tool_get_lint_rules(const Dictionary &p_args) {
	Array rules;

	auto add_rule = [&](const String &p_name, const String &p_severity, const String &p_desc, bool p_auto_fixable) {
		Dictionary r;
		r["name"] = p_name;
		r["severity"] = p_severity;
		r["description"] = p_desc;
		r["auto_fixable"] = p_auto_fixable;
		rules.push_back(r);
	};

	add_rule("UNTYPED_DECLARATION", "warning", "Variables or parameters declared without type annotations", true);
	add_rule("UNUSED_VARIABLE", "warning", "Variables declared but never referenced elsewhere in the script", true);
	add_rule("MISSING_RETURN_TYPE", "warning", "Functions declared without a -> return type annotation", true);
	add_rule("MISSING_DOCSTRING", "info", "Public functions (not starting with _) without a ## documentation comment", false);
	add_rule("SIGNAL_NAMING", "warning", "Signals not following snake_case naming convention", true);
	add_rule("NODE_PATH_LITERAL", "info", "Hardcoded NodePath strings; suggest @export or %UniqueNode", false);
	add_rule("LONG_FUNCTION", "warning", "Functions exceeding 50 lines of code", false);
	add_rule("CYCLOMATIC_COMPLEXITY", "warning", "Functions with more than 10 branching statements (if/elif/match/while/for)", false);

	Dictionary result = mcp_success();
	result["rules"] = rules;
	result["count"] = rules.size();
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_linter_tools(MCPServer *p_server) {
	// lint_script
	{
		Dictionary schema = MCPSchema::object()
									.prop("path", "string", "Path to the GDScript file to lint (e.g., res://player.gd)", true)
									.build();

		p_server->register_tool("lint_script",
				"Run static analysis linting rules on a GDScript file and return diagnostics",
				schema, callable_mp_static(_tool_lint_script));
	}

	// lint_project
	{
		Dictionary schema = MCPSchema::object()
									.build();

		p_server->register_tool("lint_project",
				"Lint all .gd files in the project and return aggregated diagnostics",
				schema, callable_mp_static(_tool_lint_project));
	}

	// get_lint_rules
	{
		Dictionary schema = MCPSchema::object()
									.build();

		p_server->register_tool("get_lint_rules",
				"List all available linting rules with their descriptions and severity levels",
				schema, callable_mp_static(_tool_get_lint_rules));
	}
}
