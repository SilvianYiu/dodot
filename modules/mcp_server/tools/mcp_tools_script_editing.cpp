/**************************************************************************/
/*  mcp_tools_script_editing.cpp                                          */
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

#include "mcp_tools_script_editing.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/io/file_access.h"

// --- Tool Handlers ---

static Dictionary mcp_tool_patch_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	String old_text = p_args.get("old_text", "");
	String new_text = p_args.get("new_text", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}
	if (old_text.is_empty()) {
		return mcp_error("old_text is required");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	// Read existing content.
	String content = FileAccess::get_file_as_string(path);

	// Find the old text.
	int pos = content.find(old_text);
	if (pos == -1) {
		return mcp_error("old_text not found in file: " + path);
	}

	// Check for ambiguous matches (old_text appears more than once).
	int second_pos = content.find(old_text, pos + old_text.length());
	if (second_pos != -1) {
		return mcp_error("old_text matches multiple locations in file. Provide more context to make the match unique.");
	}

	// Perform the replacement.
	String patched = content.substr(0, pos) + new_text + content.substr(pos + old_text.length());

	// Write back.
	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_null()) {
		return mcp_error("Failed to open file for writing: " + path);
	}
	fa->store_string(patched);
	fa.unref();

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	result["replaced_at_position"] = pos;
	return result;
}

static Dictionary mcp_tool_insert_at_line(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	int line = p_args.get("line", -1);
	String content = p_args.get("content", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}
	if (line < 1) {
		return mcp_error("line must be a positive integer (1-based)");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	// Read existing content.
	String file_content = FileAccess::get_file_as_string(path);
	Vector<String> lines = file_content.split("\n");

	// Clamp line to valid range. Line is 1-based; insertion before that line.
	int insert_idx = line - 1;
	if (insert_idx > lines.size()) {
		insert_idx = lines.size();
	}

	// Split the content to insert into lines.
	Vector<String> new_lines = content.split("\n");

	// Insert the new lines.
	for (int i = 0; i < new_lines.size(); i++) {
		if (insert_idx + i <= lines.size()) {
			lines.insert(insert_idx + i, new_lines[i]);
		} else {
			lines.push_back(new_lines[i]);
		}
	}

	// Rejoin and write.
	String result_content = String("\n").join(lines);

	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_null()) {
		return mcp_error("Failed to open file for writing: " + path);
	}
	fa->store_string(result_content);
	fa.unref();

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	result["inserted_at_line"] = line;
	result["lines_inserted"] = new_lines.size();
	result["total_lines"] = lines.size();
	return result;
}

static Dictionary mcp_tool_delete_lines(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	int from_line = p_args.get("from_line", -1);
	int to_line = p_args.get("to_line", -1);

	if (path.is_empty()) {
		return mcp_error("path is required");
	}
	if (from_line < 1) {
		return mcp_error("from_line must be a positive integer (1-based)");
	}
	if (to_line < from_line) {
		return mcp_error("to_line must be >= from_line");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	// Read existing content.
	String file_content = FileAccess::get_file_as_string(path);
	Vector<String> lines = file_content.split("\n");

	// Convert to 0-based indices.
	int from_idx = from_line - 1;
	int to_idx = to_line - 1;

	if (from_idx >= lines.size()) {
		return mcp_error("from_line exceeds file length (" + itos(lines.size()) + " lines)");
	}
	if (to_idx >= lines.size()) {
		to_idx = lines.size() - 1;
	}

	int count = to_idx - from_idx + 1;

	// Remove the lines.
	for (int i = 0; i < count; i++) {
		lines.remove_at(from_idx);
	}

	// Rejoin and write.
	String result_content = String("\n").join(lines);

	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_null()) {
		return mcp_error("Failed to open file for writing: " + path);
	}
	fa->store_string(result_content);
	fa.unref();

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	result["deleted_from"] = from_line;
	result["deleted_to"] = to_idx + 1;
	result["lines_deleted"] = count;
	result["total_lines"] = lines.size();
	return result;
}

static Dictionary mcp_tool_get_script_content(const Dictionary &p_args) {
	String path = p_args.get("path", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	String content = FileAccess::get_file_as_string(path);

	// Count lines.
	Vector<String> lines = content.split("\n");

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	result["content"] = content;
	result["line_count"] = lines.size();
	return result;
}

// --- Registration ---

void mcp_register_script_editing_tools(MCPServer *p_server) {
	p_server->register_tool("patch_script", "Find and replace text in a script file. The old_text must match exactly once in the file.",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file (e.g. res://player.gd)", true)
					.prop("old_text", "string", "Exact text to find and replace (must be unique in the file)", true)
					.prop("new_text", "string", "Replacement text", true)
					.build(),
			callable_mp_static(&mcp_tool_patch_script));

	p_server->register_tool("insert_at_line", "Insert content at a specific line number in a script file",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file (e.g. res://player.gd)", true)
					.prop_number("line", "Line number to insert before (1-based)", true)
					.prop("content", "string", "Content to insert (can contain newlines)", true)
					.build(),
			callable_mp_static(&mcp_tool_insert_at_line));

	p_server->register_tool("delete_lines", "Delete a range of lines from a script file",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file (e.g. res://player.gd)", true)
					.prop_number("from_line", "First line to delete (1-based, inclusive)", true)
					.prop_number("to_line", "Last line to delete (1-based, inclusive)", true)
					.build(),
			callable_mp_static(&mcp_tool_delete_lines));

	p_server->register_tool("get_script_content", "Read the entire content of a script file",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file (e.g. res://player.gd)", true)
					.build(),
			callable_mp_static(&mcp_tool_get_script_content));
}
