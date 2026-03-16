/**************************************************************************/
/*  structured_logger.cpp                                                 */
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

#include "structured_logger.h"

#include "core/object/script_backtrace.h"

#include <cstdio>
#include <ctime>

String StructuredLogger::_get_iso_timestamp() {
	time_t now = time(nullptr);
	struct tm utc_tm;
#if defined(_MSC_VER) || defined(MINGW_ENABLED)
	gmtime_s(&utc_tm, &now);
#else
	gmtime_r(&now, &utc_tm);
#endif
	char buf[32];
	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc_tm);
	return String::utf8(buf);
}

String StructuredLogger::_escape_json_string(const String &p_str) {
	String result;
	for (int i = 0; i < p_str.length(); i++) {
		char32_t c = p_str[i];
		switch (c) {
			case '\"':
				result += "\\\"";
				break;
			case '\\':
				result += "\\\\";
				break;
			case '\b':
				result += "\\b";
				break;
			case '\f':
				result += "\\f";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				if (c < 0x20) {
					char hex[8];
					snprintf(hex, sizeof(hex), "\\u%04x", (unsigned int)c);
					result += String::utf8(hex);
				} else {
					result += c;
				}
				break;
		}
	}
	return result;
}

void StructuredLogger::_emit_json_line(const String &p_json, bool p_err) {
	CharString utf8 = p_json.utf8();
	FILE *out = p_err ? stderr : stdout;
	fprintf(out, "%s\n", utf8.get_data());
	fflush(out);
}

void StructuredLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	if (!should_log(p_err)) {
		return;
	}

	const int static_buf_size = 512;
	char static_buf[static_buf_size];
	char *buf = static_buf;
	va_list list_copy;
	va_copy(list_copy, p_list);
	int len = vsnprintf(buf, static_buf_size, p_format, p_list);
	if (len >= static_buf_size) {
		buf = (char *)Memory::alloc_static(len + 1);
		vsnprintf(buf, len + 1, p_format, list_copy);
	}
	va_end(list_copy);

	String message = String::utf8(buf, len);
	if (len >= static_buf_size) {
		Memory::free_static(buf);
	}

	// Strip trailing newlines since JSON Lines adds its own.
	while (message.length() > 0 && (message[message.length() - 1] == '\n' || message[message.length() - 1] == '\r')) {
		message = message.substr(0, message.length() - 1);
	}

	if (message.is_empty()) {
		return;
	}

	String json = "{\"type\":\"";
	json += p_err ? "warning" : "info";
	json += "\",\"timestamp\":\"";
	json += _get_iso_timestamp();
	json += "\",\"message\":\"";
	json += _escape_json_string(message);
	json += "\"}";

	MutexLock lock(mutex);
	_emit_json_line(json, p_err);
}

void StructuredLogger::log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify, ErrorType p_type, const Vector<Ref<ScriptBacktrace>> &p_script_backtraces) {
	if (!should_log(true)) {
		return;
	}

	String type_str;
	switch (p_type) {
		case ERR_ERROR:
			type_str = "error";
			break;
		case ERR_WARNING:
			type_str = "warning";
			break;
		case ERR_SCRIPT:
			type_str = "script_error";
			break;
		case ERR_SHADER:
			type_str = "shader_error";
			break;
	}

	String error_msg = p_code ? String::utf8(p_code) : String();
	String rationale_msg = p_rationale ? String::utf8(p_rationale) : String();

	String json = "{\"type\":\"";
	json += _escape_json_string(type_str);
	json += "\",\"timestamp\":\"";
	json += _get_iso_timestamp();
	json += "\",\"function\":\"";
	json += _escape_json_string(p_function ? String::utf8(p_function) : String());
	json += "\",\"file\":\"";
	json += _escape_json_string(p_file ? String::utf8(p_file) : String());
	json += "\",\"line\":";
	json += itos(p_line);
	json += ",\"error\":\"";
	json += _escape_json_string(error_msg);
	json += "\",\"message\":\"";
	json += _escape_json_string(rationale_msg.is_empty() ? error_msg : rationale_msg);
	json += "\"";

	if (!p_script_backtraces.is_empty()) {
		json += ",\"backtrace\":[";
		bool first = true;
		for (const Ref<ScriptBacktrace> &backtrace : p_script_backtraces) {
			if (!backtrace->is_empty()) {
				if (!first) {
					json += ",";
				}
				json += "\"";
				json += _escape_json_string(backtrace->format(0));
				json += "\"";
				first = false;
			}
		}
		json += "]";
	}

	json += "}";

	MutexLock lock(mutex);
	_emit_json_line(json, true);
}
