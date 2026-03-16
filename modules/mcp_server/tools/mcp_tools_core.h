/**************************************************************************/
/*  mcp_tools_core.h                                                      */
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

#pragma once

#include "core/variant/dictionary.h"

class MCPServer;

void mcp_register_core_tools(MCPServer *p_server);

// Tool handler declarations.
static Dictionary mcp_tool_ping(const Dictionary &p_args);
static Dictionary mcp_tool_get_engine_info(const Dictionary &p_args);
static Dictionary mcp_tool_get_project_structure(const Dictionary &p_args);
static Dictionary mcp_tool_get_scene_tree(const Dictionary &p_args);
static Dictionary mcp_tool_create_node(const Dictionary &p_args);
static Dictionary mcp_tool_delete_node(const Dictionary &p_args);
static Dictionary mcp_tool_modify_node(const Dictionary &p_args);
static Dictionary mcp_tool_save_scene(const Dictionary &p_args);
static Dictionary mcp_tool_load_scene(const Dictionary &p_args);
static Dictionary mcp_tool_create_script(const Dictionary &p_args);
static Dictionary mcp_tool_edit_script(const Dictionary &p_args);
static Dictionary mcp_tool_validate_script(const Dictionary &p_args);
static Dictionary mcp_tool_get_script_symbols(const Dictionary &p_args);
static Dictionary mcp_tool_attach_script(const Dictionary &p_args);
static Dictionary mcp_tool_get_project_settings(const Dictionary &p_args);
static Dictionary mcp_tool_set_project_setting(const Dictionary &p_args);
static Dictionary mcp_tool_run_project(const Dictionary &p_args);
static Dictionary mcp_tool_stop_project(const Dictionary &p_args);
static Dictionary mcp_tool_run_scene(const Dictionary &p_args);
static Dictionary mcp_tool_get_debug_output(const Dictionary &p_args);
static Dictionary mcp_tool_get_errors(const Dictionary &p_args);
static Dictionary mcp_tool_get_editor_state(const Dictionary &p_args);
static Dictionary mcp_tool_take_screenshot(const Dictionary &p_args);
