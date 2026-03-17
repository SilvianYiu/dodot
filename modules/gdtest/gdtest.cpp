/**************************************************************************/
/*  gdtest.cpp                                                            */
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

#include "gdtest.h"

#include "core/io/dir_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "core/os/os.h"

// =============================================================================
// GDTestResult
// =============================================================================

void GDTestResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_test_name"), &GDTestResult::get_test_name);
	ClassDB::bind_method(D_METHOD("set_test_name", "name"), &GDTestResult::set_test_name);
	ClassDB::bind_method(D_METHOD("get_suite_name"), &GDTestResult::get_suite_name);
	ClassDB::bind_method(D_METHOD("set_suite_name", "name"), &GDTestResult::set_suite_name);
	ClassDB::bind_method(D_METHOD("get_status"), &GDTestResult::get_status);
	ClassDB::bind_method(D_METHOD("set_status", "status"), &GDTestResult::set_status);
	ClassDB::bind_method(D_METHOD("get_message"), &GDTestResult::get_message);
	ClassDB::bind_method(D_METHOD("set_message", "message"), &GDTestResult::set_message);
	ClassDB::bind_method(D_METHOD("get_file_path"), &GDTestResult::get_file_path);
	ClassDB::bind_method(D_METHOD("set_file_path", "path"), &GDTestResult::set_file_path);
	ClassDB::bind_method(D_METHOD("get_duration_ms"), &GDTestResult::get_duration_ms);
	ClassDB::bind_method(D_METHOD("set_duration_ms", "ms"), &GDTestResult::set_duration_ms);
	ClassDB::bind_method(D_METHOD("get_assertions"), &GDTestResult::get_assertions);
	ClassDB::bind_method(D_METHOD("to_dict"), &GDTestResult::to_dict);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "test_name"), "set_test_name", "get_test_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "suite_name"), "set_suite_name", "get_suite_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "status"), "set_status", "get_status");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "file_path"), "set_file_path", "get_file_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration_ms"), "set_duration_ms", "get_duration_ms");

	BIND_ENUM_CONSTANT(PASSED);
	BIND_ENUM_CONSTANT(FAILED);
	BIND_ENUM_CONSTANT(ERROR);
	BIND_ENUM_CONSTANT(SKIPPED);
}

Dictionary GDTestResult::to_dict() const {
	Dictionary d;
	d["test_name"] = test_name;
	d["suite_name"] = suite_name;
	switch (status) {
		case PASSED:
			d["status"] = "passed";
			break;
		case FAILED:
			d["status"] = "failed";
			break;
		case ERROR:
			d["status"] = "error";
			break;
		case SKIPPED:
			d["status"] = "skipped";
			break;
	}
	d["message"] = message;
	d["file_path"] = file_path;
	d["duration_ms"] = duration_ms;
	if (!assertions.is_empty()) {
		d["assertions"] = assertions;
	}
	return d;
}

// =============================================================================
// GDTest
// =============================================================================

void GDTest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("run_all"), &GDTest::run_all);
	ClassDB::bind_method(D_METHOD("run_file", "path"), &GDTest::run_file);
	ClassDB::bind_method(D_METHOD("discover_tests"), &GDTest::discover_tests);

	ClassDB::bind_method(D_METHOD("set_test_dir", "dir"), &GDTest::set_test_dir);
	ClassDB::bind_method(D_METHOD("get_test_dir"), &GDTest::get_test_dir);
	ClassDB::bind_method(D_METHOD("set_test_prefix", "prefix"), &GDTest::set_test_prefix);
	ClassDB::bind_method(D_METHOD("get_test_prefix"), &GDTest::get_test_prefix);
	ClassDB::bind_method(D_METHOD("set_method_prefix", "prefix"), &GDTest::set_method_prefix);
	ClassDB::bind_method(D_METHOD("get_method_prefix"), &GDTest::get_method_prefix);
	ClassDB::bind_method(D_METHOD("set_verbose", "verbose"), &GDTest::set_verbose);
	ClassDB::bind_method(D_METHOD("get_verbose"), &GDTest::get_verbose);

	ClassDB::bind_method(D_METHOD("get_passed"), &GDTest::get_passed);
	ClassDB::bind_method(D_METHOD("get_failed"), &GDTest::get_failed);
	ClassDB::bind_method(D_METHOD("get_errors"), &GDTest::get_errors);
	ClassDB::bind_method(D_METHOD("get_skipped"), &GDTest::get_skipped);
	ClassDB::bind_method(D_METHOD("get_total"), &GDTest::get_total);
	ClassDB::bind_method(D_METHOD("get_total_duration_ms"), &GDTest::get_total_duration_ms);
	ClassDB::bind_method(D_METHOD("get_results"), &GDTest::get_results);
	ClassDB::bind_method(D_METHOD("to_json"), &GDTest::to_json);
	ClassDB::bind_method(D_METHOD("to_summary"), &GDTest::to_summary);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "test_dir"), "set_test_dir", "get_test_dir");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "test_prefix"), "set_test_prefix", "get_test_prefix");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "method_prefix"), "set_method_prefix", "get_method_prefix");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "verbose"), "set_verbose", "get_verbose");
}

// --- Test Discovery ---

Vector<String> GDTest::_discover_test_files(const String &p_dir, int p_depth) {
	Vector<String> test_files;

	const int MAX_DEPTH = 20;
	if (p_depth > MAX_DEPTH) {
		WARN_PRINT(vformat("Test directory nesting too deep (>%d) at '%s', skipping.", MAX_DEPTH, p_dir));
		return test_files;
	}

	Ref<DirAccess> da = DirAccess::open(p_dir);
	if (da.is_null()) {
		return test_files;
	}

	// Scan for test files.
	da->list_dir_begin();
	String file = da->get_next();
	while (!file.is_empty()) {
		if (da->current_is_dir()) {
			if (file != "." && file != "..") {
				// Recurse into subdirectories.
				String subdir = p_dir.path_join(file);
				Vector<String> sub_files = _discover_test_files(subdir, p_depth + 1);
				test_files.append_array(sub_files);
			}
		} else if (file.ends_with(".gd") && file.begins_with(test_prefix)) {
			test_files.push_back(p_dir.path_join(file));
		}
		file = da->get_next();
	}
	da->list_dir_end();

	test_files.sort();
	return test_files;
}

Array GDTest::discover_tests() {
	Array discovered;
	Vector<String> files = _discover_test_files(test_dir);
	for (const String &f : files) {
		discovered.push_back(f);
	}
	return discovered;
}

// --- Test Execution ---

Ref<GDTestResult> GDTest::_execute_test_method(Object *p_instance, const String &p_method, const String &p_suite, const String &p_path) {
	Ref<GDTestResult> result;
	result.instantiate();
	result->set_test_name(p_method);
	result->set_suite_name(p_suite);
	result->set_file_path(p_path);

	uint64_t start = OS::get_singleton()->get_ticks_usec();

	// Call the test method. If it doesn't exist, call() returns Variant() silently.
	if (!p_instance->has_method(StringName(p_method))) {
		result->set_status(GDTestResult::ERROR);
		result->set_message("Method not found: " + p_method);
		return result;
	}

	Variant ret = p_instance->call(StringName(p_method));

	uint64_t elapsed = OS::get_singleton()->get_ticks_usec() - start;
	result->set_duration_ms(elapsed / 1000.0);

	// Check return value — if the test returns a Dictionary, it may contain assertion results.
	if (ret.get_type() == Variant::DICTIONARY) {
		Dictionary ret_dict = ret;
		if (ret_dict.has("status")) {
			String status_str = ret_dict.get("status", "passed");
			if (status_str == "failed") {
				result->set_status(GDTestResult::FAILED);
				result->set_message(ret_dict.get("message", "Test failed"));
			} else if (status_str == "skipped") {
				result->set_status(GDTestResult::SKIPPED);
				result->set_message(ret_dict.get("message", "Test skipped"));
			}
		}
		if (ret_dict.has("assertions")) {
			Array assertions = ret_dict["assertions"];
			for (int i = 0; i < assertions.size(); i++) {
				result->add_assertion(assertions[i]);
			}
		}
	} else if (ret.get_type() == Variant::BOOL && !(bool)ret) {
		// If test returns false, it failed.
		result->set_status(GDTestResult::FAILED);
		result->set_message("Test returned false");
	}

	return result;
}

Array GDTest::_run_test_script(const String &p_path) {
	Array script_results;

	// Load the script.
	Ref<Script> script = ResourceLoader::load(p_path, "GDScript");
	if (script.is_null()) {
		Ref<GDTestResult> result;
		result.instantiate();
		result->set_test_name("load");
		result->set_suite_name(p_path.get_file().get_basename());
		result->set_file_path(p_path);
		result->set_status(GDTestResult::ERROR);
		result->set_message("Failed to load script: " + p_path);
		script_results.push_back(result);
		return script_results;
	}

	// Check for errors.
	if (!script->can_instantiate()) {
		Ref<GDTestResult> result;
		result.instantiate();
		result->set_test_name("compile");
		result->set_suite_name(p_path.get_file().get_basename());
		result->set_file_path(p_path);
		result->set_status(GDTestResult::ERROR);
		result->set_message("Script cannot be instantiated (compile error?): " + p_path);
		script_results.push_back(result);
		return script_results;
	}

	// Instantiate.
	StringName base_type = script->get_instance_base_type();
	Object *obj = ClassDB::instantiate(base_type);
	Ref<RefCounted> ref_holder; // Prevents leak if obj is RefCounted.
	if (!obj) {
		ref_holder.instantiate();
		obj = ref_holder.ptr();
	} else {
		RefCounted *rc = Object::cast_to<RefCounted>(obj);
		if (rc) {
			ref_holder = Ref<RefCounted>(rc);
		}
	}

	obj->set_script(script);

	String suite_name = p_path.get_file().get_basename();

	// Call before_all if it exists.
	if (obj->has_method("before_all")) {
		obj->call(StringName("before_all"));
	}

	// Discover test methods.
	List<MethodInfo> methods;
	script->get_script_method_list(&methods);

	for (const MethodInfo &mi : methods) {
		String method_name = mi.name;
		if (!method_name.begins_with(method_prefix)) {
			continue;
		}

		// Call before_each if it exists.
		if (obj->has_method("before_each")) {
			obj->call(StringName("before_each"));
		}

		// Execute test.
		Ref<GDTestResult> result = _execute_test_method(obj, method_name, suite_name, p_path);
		script_results.push_back(result);

		// Call after_each if it exists.
		if (obj->has_method("after_each")) {
			obj->call(StringName("after_each"));
		}
	}

	// Call after_all if it exists.
	if (obj->has_method("after_all")) {
		obj->call(StringName("after_all"));
	}

	// If the object is not RefCounted, we need to free it.
	// RefCounted objects are cleaned up automatically via ref_holder.
	if (ref_holder.is_null()) {
		memdelete(obj);
	}

	return script_results;
}

int GDTest::run_all() {
	// Reset state.
	results.clear();
	passed = 0;
	failed = 0;
	errors = 0;
	skipped = 0;
	total_duration_ms = 0.0;

	uint64_t start = OS::get_singleton()->get_ticks_usec();

	Vector<String> test_files = _discover_test_files(test_dir);

	if (verbose) {
		OS::get_singleton()->print("GDTest: Found %d test file(s) in %s\n", test_files.size(), test_dir.utf8().get_data());
	}

	for (const String &path : test_files) {
		if (verbose) {
			OS::get_singleton()->print("  Running: %s\n", path.utf8().get_data());
		}

		Array file_results = _run_test_script(path);
		for (int i = 0; i < file_results.size(); i++) {
			Ref<GDTestResult> result = file_results[i];
			results.push_back(result);

			switch (result->get_status()) {
				case GDTestResult::PASSED:
					passed++;
					break;
				case GDTestResult::FAILED:
					failed++;
					break;
				case GDTestResult::ERROR:
					errors++;
					break;
				case GDTestResult::SKIPPED:
					skipped++;
					break;
			}

			if (verbose) {
				String status_str;
				switch (result->get_status()) {
					case GDTestResult::PASSED:
						status_str = "PASS";
						break;
					case GDTestResult::FAILED:
						status_str = "FAIL";
						break;
					case GDTestResult::ERROR:
						status_str = "ERROR";
						break;
					case GDTestResult::SKIPPED:
						status_str = "SKIP";
						break;
				}
				OS::get_singleton()->print("    [%s] %s::%s (%.1fms)\n",
						status_str.utf8().get_data(),
						result->get_suite_name().utf8().get_data(),
						result->get_test_name().utf8().get_data(),
						result->get_duration_ms());
				if (!result->get_message().is_empty()) {
					OS::get_singleton()->print("           %s\n", result->get_message().utf8().get_data());
				}
			}
		}
	}

	total_duration_ms = (OS::get_singleton()->get_ticks_usec() - start) / 1000.0;

	return failed + errors;
}

int GDTest::run_file(const String &p_path) {
	results.clear();
	passed = 0;
	failed = 0;
	errors = 0;
	skipped = 0;

	uint64_t start = OS::get_singleton()->get_ticks_usec();

	Array file_results = _run_test_script(p_path);
	for (int i = 0; i < file_results.size(); i++) {
		Ref<GDTestResult> result = file_results[i];
		results.push_back(result);

		switch (result->get_status()) {
			case GDTestResult::PASSED:
				passed++;
				break;
			case GDTestResult::FAILED:
				failed++;
				break;
			case GDTestResult::ERROR:
				errors++;
				break;
			case GDTestResult::SKIPPED:
				skipped++;
				break;
		}
	}

	total_duration_ms = (OS::get_singleton()->get_ticks_usec() - start) / 1000.0;

	return failed + errors;
}

// --- Output ---

Dictionary GDTest::to_json() const {
	Dictionary output;
	output["framework"] = "GDTest";
	output["test_dir"] = test_dir;

	Dictionary summary;
	summary["total"] = get_total();
	summary["passed"] = passed;
	summary["failed"] = failed;
	summary["errors"] = errors;
	summary["skipped"] = skipped;
	summary["duration_ms"] = total_duration_ms;
	summary["success"] = (failed == 0 && errors == 0);
	output["summary"] = summary;

	Array test_results;
	for (int i = 0; i < results.size(); i++) {
		Ref<GDTestResult> result = results[i];
		if (result.is_valid()) {
			test_results.push_back(result->to_dict());
		}
	}
	output["tests"] = test_results;

	return output;
}

String GDTest::to_summary() const {
	String s;
	s += "GDTest Results\n";
	s += "==============\n";

	for (int i = 0; i < results.size(); i++) {
		Ref<GDTestResult> result = results[i];
		if (result.is_null()) {
			continue;
		}

		String icon;
		switch (result->get_status()) {
			case GDTestResult::PASSED:
				icon = "PASS";
				break;
			case GDTestResult::FAILED:
				icon = "FAIL";
				break;
			case GDTestResult::ERROR:
				icon = "ERR ";
				break;
			case GDTestResult::SKIPPED:
				icon = "SKIP";
				break;
		}

		s += vformat("[%s] %s::%s (%.1fms)\n", icon, result->get_suite_name(), result->get_test_name(), result->get_duration_ms());
		if (!result->get_message().is_empty()) {
			s += vformat("       %s\n", result->get_message());
		}
	}

	s += vformat("\n%d tests: %d passed, %d failed, %d errors, %d skipped (%.1fms)\n",
			get_total(), passed, failed, errors, skipped, total_duration_ms);

	return s;
}

// --- Static CLI Helper ---

int GDTest::run_tests_cli(const String &p_dir, bool p_verbose) {
	Ref<GDTest> runner;
	runner.instantiate();
	runner->set_test_dir(p_dir.is_empty() ? "res://tests" : p_dir);
	runner->set_verbose(p_verbose);

	int failures = runner->run_all();

	// Output JSON.
	Dictionary json = runner->to_json();
	String json_str = JSON::stringify(json, "\t");
	OS::get_singleton()->print("%s\n", json_str.utf8().get_data());

	return failures > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
