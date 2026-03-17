/**************************************************************************/
/*  gdtest.h                                                              */
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

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

// Individual test result.
class GDTestResult : public RefCounted {
	GDCLASS(GDTestResult, RefCounted);

public:
	enum Status {
		PASSED,
		FAILED,
		ERROR,
		SKIPPED,
	};

private:
	String test_name;
	String suite_name;
	Status status = PASSED;
	String message;
	String file_path;
	double duration_ms = 0.0;
	Array assertions;

protected:
	static void _bind_methods();

public:
	void set_test_name(const String &p_name) { test_name = p_name; }
	String get_test_name() const { return test_name; }

	void set_suite_name(const String &p_name) { suite_name = p_name; }
	String get_suite_name() const { return suite_name; }

	void set_status(Status p_status) { status = p_status; }
	Status get_status() const { return status; }

	void set_message(const String &p_msg) { message = p_msg; }
	String get_message() const { return message; }

	void set_file_path(const String &p_path) { file_path = p_path; }
	String get_file_path() const { return file_path; }

	void set_duration_ms(double p_ms) { duration_ms = p_ms; }
	double get_duration_ms() const { return duration_ms; }

	void add_assertion(const Dictionary &p_assertion) { assertions.push_back(p_assertion); }
	Array get_assertions() const { return assertions; }

	Dictionary to_dict() const;
};

VARIANT_ENUM_CAST(GDTestResult::Status);

// Test runner — discovers and executes GDScript tests.
class GDTest : public RefCounted {
	GDCLASS(GDTest, RefCounted);

	// Configuration.
	String test_dir = "res://tests";
	String test_prefix = "test_";
	String method_prefix = "test_";
	bool verbose = false;

	// Results.
	Array results; // Array of Ref<GDTestResult>
	int passed = 0;
	int failed = 0;
	int errors = 0;
	int skipped = 0;
	double total_duration_ms = 0.0;

	// Internal.
	Vector<String> _discover_test_files(const String &p_dir, int p_depth = 0);
	Array _run_test_script(const String &p_path);
	Ref<GDTestResult> _execute_test_method(Object *p_instance, const String &p_method, const String &p_suite, const String &p_path);

protected:
	static void _bind_methods();

public:
	// Configuration.
	void set_test_dir(const String &p_dir) { test_dir = p_dir; }
	String get_test_dir() const { return test_dir; }

	void set_test_prefix(const String &p_prefix) { test_prefix = p_prefix; }
	String get_test_prefix() const { return test_prefix; }

	void set_method_prefix(const String &p_prefix) { method_prefix = p_prefix; }
	String get_method_prefix() const { return method_prefix; }

	void set_verbose(bool p_verbose) { verbose = p_verbose; }
	bool get_verbose() const { return verbose; }

	// Execution.
	int run_all();
	int run_file(const String &p_path);
	Array discover_tests();

	// Results.
	int get_passed() const { return passed; }
	int get_failed() const { return failed; }
	int get_errors() const { return errors; }
	int get_skipped() const { return skipped; }
	int get_total() const { return passed + failed + errors + skipped; }
	double get_total_duration_ms() const { return total_duration_ms; }
	Array get_results() const { return results; }

	// Output.
	Dictionary to_json() const;
	String to_summary() const;

	// Static convenience for CLI.
	static int run_tests_cli(const String &p_dir, bool p_verbose);
};
