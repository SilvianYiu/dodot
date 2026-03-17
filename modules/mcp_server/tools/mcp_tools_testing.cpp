/**************************************************************************/
/*  mcp_tools_testing.cpp                                                 */
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

#include "mcp_tools_testing.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#ifdef MODULE_GDTEST_ENABLED
#include "modules/gdtest/gdtest.h"
#endif

// --- Tool: run_tests ---
static Dictionary mcp_tool_run_tests(const Dictionary &p_args) {
#ifdef MODULE_GDTEST_ENABLED
	String dir = p_args.get("directory", "res://tests");
	String file = p_args.get("file", "");

	Ref<GDTest> runner;
	runner.instantiate();
	runner->set_test_dir(dir);
	runner->set_verbose(false);

	if (!file.is_empty()) {
		runner->run_file(file);
	} else {
		runner->run_all();
	}

	return mcp_success(runner->to_json());
#else
	return mcp_error("GDTest module not enabled");
#endif
}

// --- Tool: discover_tests ---
static Dictionary mcp_tool_discover_tests(const Dictionary &p_args) {
#ifdef MODULE_GDTEST_ENABLED
	String dir = p_args.get("directory", "res://tests");

	Ref<GDTest> runner;
	runner.instantiate();
	runner->set_test_dir(dir);

	Array tests = runner->discover_tests();

	Dictionary result;
	result["directory"] = dir;
	result["test_files"] = tests;
	result["count"] = tests.size();
	return mcp_success(result);
#else
	return mcp_error("GDTest module not enabled");
#endif
}

// --- Registration ---

void mcp_register_testing_tools(MCPServer *p_server) {
	p_server->register_tool("run_tests",
			"Run GDScript tests and return results as JSON",
			MCPSchema::object()
					.prop("directory", "string", "Test directory (default: res://tests)")
					.prop("file", "string", "Run a specific test file instead of all")
					.build(),
			callable_mp_static(&mcp_tool_run_tests));

	p_server->register_tool("discover_tests",
			"Discover all test files in the test directory",
			MCPSchema::object()
					.prop("directory", "string", "Test directory (default: res://tests)")
					.build(),
			callable_mp_static(&mcp_tool_discover_tests));
}
