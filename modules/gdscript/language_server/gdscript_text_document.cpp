/**************************************************************************/
/*  gdscript_text_document.cpp                                            */
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

#include "gdscript_text_document.h"

#include "../gdscript.h"
#include "../gdscript_analyzer.h"
#include "gdscript_extend_parser.h"
#include "gdscript_language_protocol.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/script/script_text_editor.h"
#include "editor/settings/editor_settings.h"
#include "servers/display/display_server.h"

void GDScriptTextDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_native_symbol_in_editor", "symbol_id"), &GDScriptTextDocument::show_native_symbol_in_editor);

#ifndef DISABLE_DEPRECATED
	ClassDB::bind_method(D_METHOD("didOpen", "params"), &GDScriptTextDocument::didOpen);
	ClassDB::bind_method(D_METHOD("didClose", "params"), &GDScriptTextDocument::didClose);
	ClassDB::bind_method(D_METHOD("didChange", "params"), &GDScriptTextDocument::didChange);
	ClassDB::bind_method(D_METHOD("willSaveWaitUntil", "params"), &GDScriptTextDocument::willSaveWaitUntil);
	ClassDB::bind_method(D_METHOD("didSave", "params"), &GDScriptTextDocument::didSave);
	ClassDB::bind_method(D_METHOD("nativeSymbol", "params"), &GDScriptTextDocument::nativeSymbol);
	ClassDB::bind_method(D_METHOD("documentSymbol", "params"), &GDScriptTextDocument::documentSymbol);
	ClassDB::bind_method(D_METHOD("completion", "params"), &GDScriptTextDocument::completion);
	ClassDB::bind_method(D_METHOD("resolve", "params"), &GDScriptTextDocument::resolve);
	ClassDB::bind_method(D_METHOD("rename", "params"), &GDScriptTextDocument::rename);
	ClassDB::bind_method(D_METHOD("prepareRename", "params"), &GDScriptTextDocument::prepareRename);
	ClassDB::bind_method(D_METHOD("references", "params"), &GDScriptTextDocument::references);
	ClassDB::bind_method(D_METHOD("foldingRange", "params"), &GDScriptTextDocument::foldingRange);
	ClassDB::bind_method(D_METHOD("codeLens", "params"), &GDScriptTextDocument::codeLens);
	ClassDB::bind_method(D_METHOD("documentLink", "params"), &GDScriptTextDocument::documentLink);
	ClassDB::bind_method(D_METHOD("colorPresentation", "params"), &GDScriptTextDocument::colorPresentation);
	ClassDB::bind_method(D_METHOD("hover", "params"), &GDScriptTextDocument::hover);
	ClassDB::bind_method(D_METHOD("definition", "params"), &GDScriptTextDocument::definition);
	ClassDB::bind_method(D_METHOD("declaration", "params"), &GDScriptTextDocument::declaration);
	ClassDB::bind_method(D_METHOD("signatureHelp", "params"), &GDScriptTextDocument::signatureHelp);
	ClassDB::bind_method(D_METHOD("inlayHint", "params"), &GDScriptTextDocument::inlayHint);
	ClassDB::bind_method(D_METHOD("codeAction", "params"), &GDScriptTextDocument::codeAction);
#endif // !DISABLE_DEPRECATED
}

void GDScriptTextDocument::didOpen(const Variant &p_param) {
	GDScriptLanguageProtocol::get_singleton()->lsp_did_open(p_param);
}

void GDScriptTextDocument::didChange(const Variant &p_param) {
	GDScriptLanguageProtocol::get_singleton()->lsp_did_change(p_param);
}

void GDScriptTextDocument::didClose(const Variant &p_param) {
	GDScriptLanguageProtocol::get_singleton()->lsp_did_close(p_param);
}

void GDScriptTextDocument::willSaveWaitUntil(const Variant &p_param) {
	Dictionary dict = p_param;
	LSP::TextDocumentIdentifier doc;
	doc.load(dict["textDocument"]);

	String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(doc.uri);
	Ref<Script> scr = ResourceLoader::load(path);
	if (scr.is_valid()) {
		ScriptEditor::get_singleton()->clear_docs_from_script(scr);
	}
}

void GDScriptTextDocument::didSave(const Variant &p_param) {
	Dictionary dict = p_param;
	LSP::TextDocumentIdentifier doc;
	doc.load(dict["textDocument"]);
	String text = dict["text"];

	String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(doc.uri);
	Ref<GDScript> scr = ResourceLoader::load(path);
	if (scr.is_valid() && (scr->load_source_code(path) == OK)) {
		if (scr->is_tool()) {
			scr->get_language()->reload_tool_script(scr, true);
		} else {
			scr->reload(true);
		}

		scr->update_exports();

		if (!Thread::is_main_thread()) {
			callable_mp(this, &GDScriptTextDocument::reload_script).call_deferred(scr);
		} else {
			reload_script(scr);
		}
	}
}

void GDScriptTextDocument::reload_script(Ref<GDScript> p_to_reload_script) {
	ScriptEditor::get_singleton()->reload_scripts(true);
	ScriptEditor::get_singleton()->update_docs_from_script(p_to_reload_script);
	ScriptEditor::get_singleton()->trigger_live_script_reload(p_to_reload_script->get_path());
}

void GDScriptTextDocument::notify_client_show_symbol(const LSP::DocumentSymbol *symbol) {
	ERR_FAIL_NULL(symbol);
	GDScriptLanguageProtocol::get_singleton()->notify_client("gdscript/show_native_symbol", symbol->to_json(true));
}

Variant GDScriptTextDocument::nativeSymbol(const Dictionary &p_params) {
	Variant ret;

	LSP::NativeSymbolInspectParams params;
	params.load(p_params);

	if (const LSP::DocumentSymbol *symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_native_symbol(params)) {
		ret = symbol->to_json(true);
		notify_client_show_symbol(symbol);
	}

	return ret;
}

Array GDScriptTextDocument::documentSymbol(const Dictionary &p_params) {
	Dictionary params = p_params["textDocument"];
	String uri = params["uri"];
	String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(uri);
	Array arr;

	ExtendGDScriptParser *parser = GDScriptLanguageProtocol::get_singleton()->get_parse_result(path);
	if (parser) {
		LSP::DocumentSymbol symbol = parser->get_symbols();
		arr.push_back(symbol.to_json(true));
	}
	return arr;
}

Array GDScriptTextDocument::documentHighlight(const Dictionary &p_params) {
	Array arr;
	LSP::TextDocumentPositionParams params;
	params.load(p_params);

	const LSP::DocumentSymbol *symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_symbol(params);
	if (symbol) {
		String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(params.textDocument.uri);
		Vector<LSP::Location> usages = GDScriptLanguageProtocol::get_singleton()->get_workspace()->find_usages_in_file(*symbol, path);

		for (const LSP::Location &usage : usages) {
			LSP::DocumentHighlight highlight;
			highlight.range = usage.range;
			arr.push_back(highlight.to_json());
		}
	}
	return arr;
}

Array GDScriptTextDocument::completion(const Dictionary &p_params) {
	Array arr;

	LSP::CompletionParams params;
	params.load(p_params);
	Dictionary request_data = params.to_json();

	List<ScriptLanguage::CodeCompletionOption> options;
	GDScriptLanguageProtocol::get_singleton()->get_workspace()->completion(params, &options);

	if (!options.is_empty()) {
		int i = 0;
		arr.resize(options.size());

		for (const ScriptLanguage::CodeCompletionOption &option : options) {
			LSP::CompletionItem item;
			item.label = option.display;
			item.data = request_data;
			item.insertText = option.insert_text;

			switch (option.kind) {
				case ScriptLanguage::CODE_COMPLETION_KIND_ENUM:
					item.kind = LSP::CompletionItemKind::Enum;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_CLASS:
					item.kind = LSP::CompletionItemKind::Class;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_MEMBER:
					item.kind = LSP::CompletionItemKind::Property;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_FUNCTION:
					item.kind = LSP::CompletionItemKind::Method;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_SIGNAL:
					item.kind = LSP::CompletionItemKind::Event;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_CONSTANT:
					item.kind = LSP::CompletionItemKind::Constant;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_VARIABLE:
					item.kind = LSP::CompletionItemKind::Variable;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_FILE_PATH:
					item.kind = LSP::CompletionItemKind::File;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_NODE_PATH:
					item.kind = LSP::CompletionItemKind::Snippet;
					break;
				case ScriptLanguage::CODE_COMPLETION_KIND_PLAIN_TEXT:
					item.kind = LSP::CompletionItemKind::Text;
					break;
				default: {
				}
			}

			arr[i] = item.to_json();
			i++;
		}
	}
	return arr;
}

Dictionary GDScriptTextDocument::rename(const Dictionary &p_params) {
	LSP::TextDocumentPositionParams params;
	params.load(p_params);
	String new_name = p_params["newName"];

	return GDScriptLanguageProtocol::get_singleton()->get_workspace()->rename(params, new_name);
}

Variant GDScriptTextDocument::prepareRename(const Dictionary &p_params) {
	LSP::TextDocumentPositionParams params;
	params.load(p_params);

	LSP::DocumentSymbol symbol;
	LSP::Range range;
	if (GDScriptLanguageProtocol::get_singleton()->get_workspace()->can_rename(params, symbol, range)) {
		return Variant(range.to_json());
	}

	// `null` -> rename not valid at current location.
	return Variant();
}

Array GDScriptTextDocument::references(const Dictionary &p_params) {
	Array res;

	LSP::ReferenceParams params;
	params.load(p_params);

	const LSP::DocumentSymbol *symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_symbol(params);
	if (symbol) {
		Vector<LSP::Location> usages = GDScriptLanguageProtocol::get_singleton()->get_workspace()->find_all_usages(*symbol);
		res.resize(usages.size());
		int declaration_adjustment = 0;
		for (int i = 0; i < usages.size(); i++) {
			LSP::Location usage = usages[i];
			if (!params.context.includeDeclaration && usage.range == symbol->range) {
				declaration_adjustment++;
				continue;
			}
			res[i - declaration_adjustment] = usages[i].to_json();
		}

		if (declaration_adjustment > 0) {
			res.resize(res.size() - declaration_adjustment);
		}
	}

	return res;
}

Dictionary GDScriptTextDocument::resolve(const Dictionary &p_params) {
	LSP::CompletionItem item;
	item.load(p_params);

	LSP::CompletionParams params;
	Variant data = p_params["data"];

	const LSP::DocumentSymbol *symbol = nullptr;

	if (data.get_type() == Variant::DICTIONARY) {
		params.load(p_params["data"]);
		symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_symbol(params, item.label, item.kind == LSP::CompletionItemKind::Method || item.kind == LSP::CompletionItemKind::Function);
	}

	if (symbol) {
		item.documentation = symbol->render();
	}

	if (item.kind == LSP::CompletionItemKind::Event) {
		if (params.context.triggerKind == LSP::CompletionTriggerKind::TriggerCharacter && (params.context.triggerCharacter == "(")) {
			const String quote_style = EDITOR_GET("text_editor/completion/use_single_quotes") ? "'" : "\"";
			item.insertText = item.label.quote(quote_style);
		}
	}

	if (item.kind == LSP::CompletionItemKind::Method) {
		bool is_trigger_character = params.context.triggerKind == LSP::CompletionTriggerKind::TriggerCharacter;
		bool is_quote_character = params.context.triggerCharacter == "\"" || params.context.triggerCharacter == "'";

		if (is_trigger_character && is_quote_character && item.insertText.is_quoted()) {
			item.insertText = item.insertText.unquote();
		}
	}

	return item.to_json(true);
}

Array GDScriptTextDocument::foldingRange(const Dictionary &p_params) {
	return Array();
}

Array GDScriptTextDocument::codeLens(const Dictionary &p_params) {
	return Array();
}

Array GDScriptTextDocument::documentLink(const Dictionary &p_params) {
	Array ret;

	LSP::DocumentLinkParams params;
	params.load(p_params);

	List<LSP::DocumentLink> links;
	GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_document_links(params.textDocument.uri, links);
	for (const LSP::DocumentLink &E : links) {
		ret.push_back(E.to_json());
	}
	return ret;
}

Array GDScriptTextDocument::colorPresentation(const Dictionary &p_params) {
	return Array();
}

Variant GDScriptTextDocument::hover(const Dictionary &p_params) {
	LSP::TextDocumentPositionParams params;
	params.load(p_params);

	const LSP::DocumentSymbol *symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_symbol(params);
	if (symbol) {
		LSP::Hover hover;
		hover.contents = symbol->render();
		hover.range.start = params.position;
		hover.range.end = params.position;
		return hover.to_json();

	} else if (GDScriptLanguageProtocol::get_singleton()->is_smart_resolve_enabled()) {
		Dictionary ret;
		Array contents;
		List<const LSP::DocumentSymbol *> list;
		GDScriptLanguageProtocol::get_singleton()->resolve_related_symbols(params, list);
		for (const LSP::DocumentSymbol *&E : list) {
			if (const LSP::DocumentSymbol *s = E) {
				contents.push_back(s->render().value);
			}
		}
		ret["contents"] = contents;
		return ret;
	}

	return Variant();
}

Array GDScriptTextDocument::definition(const Dictionary &p_params) {
	LSP::TextDocumentPositionParams params;
	params.load(p_params);
	List<const LSP::DocumentSymbol *> symbols;
	return find_symbols(params, symbols);
}

Variant GDScriptTextDocument::declaration(const Dictionary &p_params) {
	LSP::TextDocumentPositionParams params;
	params.load(p_params);
	List<const LSP::DocumentSymbol *> symbols;
	Array arr = find_symbols(params, symbols);
	if (arr.is_empty() && !symbols.is_empty() && !symbols.front()->get()->native_class.is_empty()) { // Find a native symbol
		const LSP::DocumentSymbol *symbol = symbols.front()->get();
		if (GDScriptLanguageProtocol::get_singleton()->is_goto_native_symbols_enabled()) {
			String id;
			switch (symbol->kind) {
				case LSP::SymbolKind::Class:
					id = "class_name:" + symbol->name;
					break;
				case LSP::SymbolKind::Constant:
					id = "class_constant:" + symbol->native_class + ":" + symbol->name;
					break;
				case LSP::SymbolKind::Property:
				case LSP::SymbolKind::Variable:
					id = "class_property:" + symbol->native_class + ":" + symbol->name;
					break;
				case LSP::SymbolKind::Enum:
					id = "class_enum:" + symbol->native_class + ":" + symbol->name;
					break;
				case LSP::SymbolKind::Method:
				case LSP::SymbolKind::Function:
					id = "class_method:" + symbol->native_class + ":" + symbol->name;
					break;
				default:
					id = "class_global:" + symbol->native_class + ":" + symbol->name;
					break;
			}
			callable_mp(this, &GDScriptTextDocument::show_native_symbol_in_editor).call_deferred(id);
		} else {
			notify_client_show_symbol(symbol);
		}
	}
	return arr;
}

Variant GDScriptTextDocument::signatureHelp(const Dictionary &p_params) {
	Variant ret;

	LSP::TextDocumentPositionParams params;
	params.load(p_params);

	LSP::SignatureHelp s;
	if (OK == GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_signature(params, s)) {
		ret = s.to_json();
	}

	return ret;
}

GDScriptTextDocument::GDScriptTextDocument() {
	file_checker = FileAccess::create(FileAccess::ACCESS_RESOURCES);
}

void GDScriptTextDocument::show_native_symbol_in_editor(const String &p_symbol_id) {
	callable_mp(ScriptEditor::get_singleton(), &ScriptEditor::goto_help).call_deferred(p_symbol_id);

	DisplayServer::get_singleton()->window_move_to_foreground();
}

// =============================================================================
// Inlay Hints
// =============================================================================

// Helper: recursively collect inlay hints from AST nodes.
static void _collect_inlay_hints_from_node(const GDScriptParser::Node *p_node, const Vector<String> &p_lines, Array &r_hints) {
	if (!p_node) {
		return;
	}

	switch (p_node->type) {
		case GDScriptParser::Node::CALL: {
			const GDScriptParser::CallNode *call = static_cast<const GDScriptParser::CallNode *>(p_node);
			// Show parameter name hints for function calls with 2+ arguments.
			if (call->arguments.size() >= 2) {
				// Try to get parameter names from the function's MethodInfo.
				// The callee's datatype may have method_info populated.
				MethodInfo mi;
				bool has_info = false;

				// Check if the call resolved to a known function via its callee's datatype.
				if (call->callee && call->callee->datatype.kind == GDScriptParser::DataType::BUILTIN) {
					Variant::Type bt = call->callee->datatype.builtin_type;
					if (bt != Variant::NIL && Variant::has_builtin_method(bt, call->function_name)) {
						mi = Variant::get_builtin_method_info(bt, call->function_name);
						has_info = true;
					}
				}

				// Try native class methods.
				if (!has_info && !call->function_name.is_empty()) {
					// Walk up to find the class context.
					if (call->callee) {
						GDScriptParser::DataType dt = call->callee->datatype;
						if (dt.kind == GDScriptParser::DataType::NATIVE && !dt.native_type.is_empty()) {
							MethodInfo m;
							if (ClassDB::get_method_info(dt.native_type, call->function_name, &m)) {
								mi = m;
								has_info = true;
							}
						}
					}
				}

				if (has_info && mi.arguments.size() > 0) {
					int hint_count = MIN(call->arguments.size(), (int)mi.arguments.size());
					for (int i = 0; i < hint_count; i++) {
						String param_name = mi.arguments[i].name;
						if (param_name.is_empty()) {
							continue;
						}

						const GDScriptParser::ExpressionNode *arg = call->arguments[i];
						if (!arg) {
							continue;
						}

						// Skip if argument is already a named literal matching the parameter.
						if (arg->type == GDScriptParser::Node::IDENTIFIER) {
							const GDScriptParser::IdentifierNode *id = static_cast<const GDScriptParser::IdentifierNode *>(arg);
							if (id->name == param_name) {
								continue;
							}
						}

						LSP::InlayHint hint;
						hint.position.line = arg->start_line - 1; // LSP is 0-based.
						hint.position.character = arg->start_column - 1;
						hint.label = param_name + ":";
						hint.kind = LSP::InlayHintKind::Parameter;
						hint.paddingRight = true;
						r_hints.push_back(hint.to_json());
					}
				}
			}

			// Recurse into call arguments.
			for (int i = 0; i < call->arguments.size(); i++) {
				_collect_inlay_hints_from_node(call->arguments[i], p_lines, r_hints);
			}
			// Recurse into callee.
			_collect_inlay_hints_from_node(call->callee, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::VARIABLE: {
			const GDScriptParser::VariableNode *var = static_cast<const GDScriptParser::VariableNode *>(p_node);
			// Show inferred type hint when no explicit type annotation.
			if (!var->datatype_specifier && var->get_datatype().is_set() && !var->get_datatype().is_variant()) {
				String type_str = var->get_datatype().to_string();
				if (!type_str.is_empty() && type_str != "Variant") {
					LSP::InlayHint hint;
					// Position after the variable name.
					if (var->identifier) {
						hint.position.line = var->identifier->start_line - 1;
						// Place after the identifier.
						hint.position.character = var->identifier->end_column;
					} else {
						hint.position.line = var->start_line - 1;
						hint.position.character = var->end_column;
					}
					hint.label = ": " + type_str;
					hint.kind = LSP::InlayHintKind::Type;
					hint.paddingLeft = true;
					r_hints.push_back(hint.to_json());
				}
			}
			break;
		}

		case GDScriptParser::Node::FOR: {
			const GDScriptParser::ForNode *for_node = static_cast<const GDScriptParser::ForNode *>(p_node);
			if (for_node->loop) {
				_collect_inlay_hints_from_node(for_node->loop, p_lines, r_hints);
			}
			_collect_inlay_hints_from_node(for_node->list, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::IF: {
			const GDScriptParser::IfNode *if_node = static_cast<const GDScriptParser::IfNode *>(p_node);
			_collect_inlay_hints_from_node(if_node->condition, p_lines, r_hints);
			if (if_node->true_block) {
				_collect_inlay_hints_from_node(if_node->true_block, p_lines, r_hints);
			}
			if (if_node->false_block) {
				_collect_inlay_hints_from_node(if_node->false_block, p_lines, r_hints);
			}
			break;
		}

		case GDScriptParser::Node::WHILE: {
			const GDScriptParser::WhileNode *while_node = static_cast<const GDScriptParser::WhileNode *>(p_node);
			_collect_inlay_hints_from_node(while_node->condition, p_lines, r_hints);
			if (while_node->loop) {
				_collect_inlay_hints_from_node(while_node->loop, p_lines, r_hints);
			}
			break;
		}

		case GDScriptParser::Node::MATCH: {
			const GDScriptParser::MatchNode *match_node = static_cast<const GDScriptParser::MatchNode *>(p_node);
			_collect_inlay_hints_from_node(match_node->test, p_lines, r_hints);
			for (int i = 0; i < match_node->branches.size(); i++) {
				if (match_node->branches[i] && match_node->branches[i]->block) {
					_collect_inlay_hints_from_node(match_node->branches[i]->block, p_lines, r_hints);
				}
			}
			break;
		}

		case GDScriptParser::Node::RETURN: {
			const GDScriptParser::ReturnNode *ret = static_cast<const GDScriptParser::ReturnNode *>(p_node);
			_collect_inlay_hints_from_node(ret->return_value, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::ASSIGNMENT: {
			const GDScriptParser::AssignmentNode *assign = static_cast<const GDScriptParser::AssignmentNode *>(p_node);
			_collect_inlay_hints_from_node(assign->assignee, p_lines, r_hints);
			_collect_inlay_hints_from_node(assign->assigned_value, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::BINARY_OPERATOR: {
			const GDScriptParser::BinaryOpNode *bin = static_cast<const GDScriptParser::BinaryOpNode *>(p_node);
			_collect_inlay_hints_from_node(bin->left_operand, p_lines, r_hints);
			_collect_inlay_hints_from_node(bin->right_operand, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::UNARY_OPERATOR: {
			const GDScriptParser::UnaryOpNode *un = static_cast<const GDScriptParser::UnaryOpNode *>(p_node);
			_collect_inlay_hints_from_node(un->operand, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::TERNARY_OPERATOR: {
			const GDScriptParser::TernaryOpNode *tern = static_cast<const GDScriptParser::TernaryOpNode *>(p_node);
			_collect_inlay_hints_from_node(tern->true_expr, p_lines, r_hints);
			_collect_inlay_hints_from_node(tern->condition, p_lines, r_hints);
			_collect_inlay_hints_from_node(tern->false_expr, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::SUBSCRIPT: {
			const GDScriptParser::SubscriptNode *sub = static_cast<const GDScriptParser::SubscriptNode *>(p_node);
			_collect_inlay_hints_from_node(sub->base, p_lines, r_hints);
			_collect_inlay_hints_from_node(sub->index, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::CAST: {
			const GDScriptParser::CastNode *cast = static_cast<const GDScriptParser::CastNode *>(p_node);
			_collect_inlay_hints_from_node(cast->operand, p_lines, r_hints);
			break;
		}

		case GDScriptParser::Node::SUITE: {
			const GDScriptParser::SuiteNode *suite = static_cast<const GDScriptParser::SuiteNode *>(p_node);
			for (int i = 0; i < suite->statements.size(); i++) {
				_collect_inlay_hints_from_node(suite->statements[i], p_lines, r_hints);
			}
			break;
		}

		case GDScriptParser::Node::FUNCTION: {
			const GDScriptParser::FunctionNode *func = static_cast<const GDScriptParser::FunctionNode *>(p_node);
			if (func->body) {
				_collect_inlay_hints_from_node(func->body, p_lines, r_hints);
			}
			break;
		}

		case GDScriptParser::Node::CLASS: {
			const GDScriptParser::ClassNode *cls = static_cast<const GDScriptParser::ClassNode *>(p_node);
			for (int i = 0; i < cls->members.size(); i++) {
				const GDScriptParser::ClassNode::Member &member = cls->members[i];
				switch (member.type) {
					case GDScriptParser::ClassNode::Member::FUNCTION:
						_collect_inlay_hints_from_node(member.function, p_lines, r_hints);
						break;
					case GDScriptParser::ClassNode::Member::VARIABLE:
						_collect_inlay_hints_from_node(member.variable, p_lines, r_hints);
						break;
					case GDScriptParser::ClassNode::Member::CLASS:
						_collect_inlay_hints_from_node(member.m_class, p_lines, r_hints);
						break;
					default:
						break;
				}
			}
			break;
		}

		default:
			break;
	}
}

Array GDScriptTextDocument::inlayHint(const Dictionary &p_params) {
	Array hints;

	Dictionary text_document = p_params["textDocument"];
	String uri = text_document["uri"];
	String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(uri);

	const ExtendGDScriptParser *parser = GDScriptLanguageProtocol::get_singleton()->get_parse_result(path);
	if (!parser) {
		return hints;
	}

	const GDScriptParser::ClassNode *tree = parser->get_tree();
	if (!tree) {
		return hints;
	}

	_collect_inlay_hints_from_node(tree, parser->get_lines(), hints);

	return hints;
}

// =============================================================================
// Code Actions
// =============================================================================

Array GDScriptTextDocument::codeAction(const Dictionary &p_params) {
	Array actions;

	Dictionary text_document = p_params["textDocument"];
	String uri = text_document["uri"];
	String path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(uri);

	Dictionary context = p_params["context"];
	Array client_diagnostics = context.get("diagnostics", Array());

	const ExtendGDScriptParser *parser = GDScriptLanguageProtocol::get_singleton()->get_parse_result(path);
	if (!parser) {
		return actions;
	}

	const Vector<LSP::Diagnostic> &diagnostics = parser->get_diagnostics();

	for (int i = 0; i < diagnostics.size(); i++) {
		const LSP::Diagnostic &diag = diagnostics[i];
		String msg = diag.message;

		// Quick fix: Remove unused variable.
		if (msg.contains("is declared but never used")) {
			// Extract variable name from message.
			// Message format: 'The local variable "x" is declared but never used...'
			int start_quote = msg.find("\"");
			int end_quote = msg.find("\"", start_quote + 1);
			if (start_quote >= 0 && end_quote > start_quote) {
				String var_name = msg.substr(start_quote + 1, end_quote - start_quote - 1);

				// Find the line and create an edit to prefix with underscore.
				int line = diag.range.start.line;
				const Vector<String> &lines = parser->get_lines();
				if (line >= 0 && line < lines.size()) {
					String line_text = lines[line];
					int var_pos = line_text.find(var_name);
					if (var_pos >= 0) {
						LSP::CodeAction action;
						action.title = "Prefix with underscore: _" + var_name;
						action.kind = LSP::CodeActionKind::QuickFix;
						action.diagnostics.push_back(diag);
						action.isPreferred = true;

						LSP::TextEdit edit;
						edit.range.start.line = line;
						edit.range.start.character = var_pos;
						edit.range.end.line = line;
						edit.range.end.character = var_pos + var_name.length();
						edit.newText = "_" + var_name;

						Vector<LSP::TextEdit> edits;
						edits.push_back(edit);
						action.edit.changes[uri] = edits;

						actions.push_back(action.to_json());
					}
				}
			}
		}

		// Quick fix: Add type annotation for untyped variables.
		if (msg.contains("has no type annotation") || msg.contains("no type annotation")) {
			int line = diag.range.start.line;
			const Vector<String> &lines = parser->get_lines();
			if (line >= 0 && line < lines.size()) {
				String line_text = lines[line];

				// Look for "var name =" pattern and suggest "var name: Type ="
				// Find the variable declaration in the AST.
				const GDScriptParser::ClassNode *tree = parser->get_tree();
				if (tree) {
					for (int m = 0; m < tree->members.size(); m++) {
						const GDScriptParser::ClassNode::Member &member = tree->members[m];
						if (member.type == GDScriptParser::ClassNode::Member::VARIABLE) {
							const GDScriptParser::VariableNode *var = member.variable;
							if (var && var->start_line - 1 == line && var->get_datatype().is_set() && !var->get_datatype().is_variant()) {
								String type_str = var->get_datatype().to_string();
								if (!type_str.is_empty() && type_str != "Variant" && var->identifier) {
									LSP::CodeAction action;
									action.title = "Add type annotation: " + type_str;
									action.kind = LSP::CodeActionKind::QuickFix;
									action.diagnostics.push_back(diag);
									action.isPreferred = true;

									// Insert ": Type" after the variable name.
									LSP::TextEdit edit;
									edit.range.start.line = line;
									edit.range.start.character = var->identifier->end_column;
									edit.range.end.line = line;
									edit.range.end.character = var->identifier->end_column;
									edit.newText = ": " + type_str;

									Vector<LSP::TextEdit> edits;
									edits.push_back(edit);
									action.edit.changes[uri] = edits;

									actions.push_back(action.to_json());
								}
							}
						}
					}
				}
			}
		}

		// Quick fix: Convert to static typing (replace = with :=).
		if (msg.contains("infer the type") || msg.contains("use := to infer")) {
			int line = diag.range.start.line;
			const Vector<String> &lines = parser->get_lines();
			if (line >= 0 && line < lines.size()) {
				String line_text = lines[line];
				int eq_pos = line_text.find("=");
				// Make sure it's not == or !=
				if (eq_pos > 0 && line_text[eq_pos - 1] != '!' && line_text[eq_pos - 1] != ':' && (eq_pos + 1 >= line_text.length() || line_text[eq_pos + 1] != '=')) {
					LSP::CodeAction action;
					action.title = "Use := for type inference";
					action.kind = LSP::CodeActionKind::QuickFix;
					action.diagnostics.push_back(diag);

					LSP::TextEdit edit;
					edit.range.start.line = line;
					edit.range.start.character = eq_pos;
					edit.range.end.line = line;
					edit.range.end.character = eq_pos + 1;
					edit.newText = ":=";

					Vector<LSP::TextEdit> edits;
					edits.push_back(edit);
					action.edit.changes[uri] = edits;

					actions.push_back(action.to_json());
				}
			}
		}
	}

	return actions;
}

Array GDScriptTextDocument::find_symbols(const LSP::TextDocumentPositionParams &p_location, List<const LSP::DocumentSymbol *> &r_list) {
	Array arr;
	const LSP::DocumentSymbol *symbol = GDScriptLanguageProtocol::get_singleton()->get_workspace()->resolve_symbol(p_location);
	if (symbol) {
		LSP::Location location;
		location.uri = symbol->uri;
		if (!location.uri.is_empty()) {
			location.range = symbol->selectionRange;
			const String &path = GDScriptLanguageProtocol::get_singleton()->get_workspace()->get_file_path(symbol->uri);
			if (file_checker->file_exists(path)) {
				arr.push_back(location.to_json());
			}
		}
		r_list.push_back(symbol);
	} else if (GDScriptLanguageProtocol::get_singleton()->is_smart_resolve_enabled()) {
		List<const LSP::DocumentSymbol *> list;
		GDScriptLanguageProtocol::get_singleton()->resolve_related_symbols(p_location, list);
		for (const LSP::DocumentSymbol *&E : list) {
			if (const LSP::DocumentSymbol *s = E) {
				if (!s->uri.is_empty()) {
					LSP::Location location;
					location.uri = s->uri;
					location.range = s->selectionRange;
					arr.push_back(location.to_json());
					r_list.push_back(s);
				}
			}
		}
	}
	return arr;
}
