/**************************************************************************/
/*  mcp_server_editor_plugin.cpp                                          */
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

#include "mcp_server_editor_plugin.h"

#include "../mcp_server.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"

int MCPServerEditorPlugin::port_override = -1;

MCPServerEditorPlugin::MCPServerEditorPlugin() {
	set_process_internal(true);
}

void MCPServerEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_EXIT_TREE: {
			stop();
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!start_attempted && EditorNode::get_singleton()->is_editor_ready()) {
				start_attempted = true;
				start();
			}

			if (started && !use_thread) {
				MCPServer::get_singleton()->poll(poll_limit_usec);
			}
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (!EditorSettings::get_singleton()->check_changed_settings_in_group("network/mcp_server")) {
				break;
			}

			String remote_host = String(_EDITOR_GET("network/mcp_server/remote_host"));
			int remote_port = (MCPServerEditorPlugin::port_override > -1) ? MCPServerEditorPlugin::port_override : (int)_EDITOR_GET("network/mcp_server/remote_port");
			bool remote_use_thread = (bool)_EDITOR_GET("network/mcp_server/use_thread");
			int remote_poll_limit = (int)_EDITOR_GET("network/mcp_server/poll_limit_usec");
			bool enabled = (bool)_EDITOR_GET("network/mcp_server/enabled");

			if (!enabled && started) {
				stop();
			} else if (enabled && (!started || remote_host != host || remote_port != port || remote_use_thread != use_thread || remote_poll_limit != poll_limit_usec)) {
				stop();
				start();
			}
		} break;
	}
}

void MCPServerEditorPlugin::thread_main(void *p_userdata) {
	set_current_thread_safe_for_nodes(true);
	MCPServerEditorPlugin *self = static_cast<MCPServerEditorPlugin *>(p_userdata);
	while (self->thread_running) {
		MCPServer::get_singleton()->poll(self->poll_limit_usec);
		OS::get_singleton()->delay_usec(50000);
	}
}

void MCPServerEditorPlugin::start() {
	bool enabled = (bool)_EDITOR_GET("network/mcp_server/enabled");
	if (!enabled) {
		return;
	}

	host = String(_EDITOR_GET("network/mcp_server/remote_host"));
	port = (MCPServerEditorPlugin::port_override > -1) ? MCPServerEditorPlugin::port_override : (int)_EDITOR_GET("network/mcp_server/remote_port");
	use_thread = (bool)_EDITOR_GET("network/mcp_server/use_thread");
	poll_limit_usec = (int)_EDITOR_GET("network/mcp_server/poll_limit_usec");

	MCPServer *mcp = MCPServer::get_singleton();
	if (!mcp) {
		return;
	}

	const Error status = mcp->start_websocket(port, host);
	if (status != OK) {
		EditorNode::get_log()->add_message("--- Failed to start MCP server on port " + itos(port) + " ---", EditorLog::MSG_TYPE_EDITOR);
		return;
	}
	EditorNode::get_log()->add_message("--- MCP server started on " + host + ":" + itos(port) + " ---", EditorLog::MSG_TYPE_EDITOR);

	if (use_thread) {
		thread_running = true;
		thread.start(MCPServerEditorPlugin::thread_main, this);
	}
	set_process_internal(!use_thread);
	started = true;

	// Connect event hooks to push editor events to MCP subscribers.
	_connect_event_hooks();
}

void MCPServerEditorPlugin::_connect_event_hooks() {
	// Connect to EditorFileSystem signals for file change events.
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs) {
		if (!efs->is_connected("filesystem_changed", callable_mp(this, &MCPServerEditorPlugin::_on_filesystem_changed))) {
			efs->connect("filesystem_changed", callable_mp(this, &MCPServerEditorPlugin::_on_filesystem_changed));
		}
		if (!efs->is_connected("resources_reload", callable_mp(this, &MCPServerEditorPlugin::_on_resources_reload))) {
			efs->connect("resources_reload", callable_mp(this, &MCPServerEditorPlugin::_on_resources_reload));
		}
	}

	// Connect to EditorNode for scene events.
	EditorNode *editor = EditorNode::get_singleton();
	if (editor) {
		if (!editor->is_connected("scene_changed", callable_mp(this, &MCPServerEditorPlugin::_on_scene_changed))) {
			editor->connect("scene_changed", callable_mp(this, &MCPServerEditorPlugin::_on_scene_changed));
		}
		if (!editor->is_connected("scene_saved", callable_mp(this, &MCPServerEditorPlugin::_on_scene_saved))) {
			editor->connect("scene_saved", callable_mp(this, &MCPServerEditorPlugin::_on_scene_saved));
		}
	}
}

void MCPServerEditorPlugin::_on_filesystem_changed() {
	MCPServer *mcp = MCPServer::get_singleton();
	if (mcp) {
		Dictionary data;
		data["event"] = "filesystem_changed";
		data["timestamp"] = OS::get_singleton()->get_ticks_msec();
		mcp->broadcast_notification("filesystem_changed", data);
	}
}

void MCPServerEditorPlugin::_on_resources_reload(const PackedStringArray &p_resources) {
	MCPServer *mcp = MCPServer::get_singleton();
	if (mcp) {
		Array paths;
		for (int i = 0; i < p_resources.size(); i++) {
			paths.push_back(p_resources[i]);
		}

		Dictionary data;
		data["event"] = "resources_reloaded";
		data["paths"] = paths;
		data["count"] = paths.size();
		data["timestamp"] = OS::get_singleton()->get_ticks_msec();
		mcp->broadcast_notification("resources_reloaded", data);
	}
}

void MCPServerEditorPlugin::_on_scene_changed() {
	MCPServer *mcp = MCPServer::get_singleton();
	if (mcp) {
		EditorNode *editor = EditorNode::get_singleton();
		Dictionary data;
		data["event"] = "scene_changed";
		if (editor && editor->get_edited_scene()) {
			data["scene_path"] = editor->get_edited_scene()->get_scene_file_path();
			data["scene_root"] = editor->get_edited_scene()->get_name();
		}
		data["timestamp"] = OS::get_singleton()->get_ticks_msec();
		mcp->broadcast_notification("scene_changed", data);
	}
}

void MCPServerEditorPlugin::_on_scene_saved(const String &p_path) {
	MCPServer *mcp = MCPServer::get_singleton();
	if (mcp) {
		Dictionary data;
		data["event"] = "scene_saved";
		data["path"] = p_path;
		data["timestamp"] = OS::get_singleton()->get_ticks_msec();
		mcp->broadcast_notification("scene_saved", data);
	}
}

void MCPServerEditorPlugin::stop() {
	if (!started) {
		return;
	}

	if (use_thread) {
		ERR_FAIL_COND(!thread.is_started());
		thread_running = false;
		thread.wait_to_finish();
	}

	MCPServer *mcp = MCPServer::get_singleton();
	if (mcp) {
		mcp->stop();
	}
	started = false;
	EditorNode::get_log()->add_message("--- MCP server stopped ---", EditorLog::MSG_TYPE_EDITOR);
}
