/**************************************************************************/
/*  mcp_tools_screenshot.cpp                                              */
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

#include "mcp_tools_screenshot.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/crypto/crypto_core.h"
#include "core/io/image.h"
#include "core/io/marshalls.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/gui/control.h"
#include "scene/main/canvas_item.h"
#include "scene/main/viewport.h"

// --- take_viewport_screenshot ---

static Dictionary mcp_tool_take_viewport_screenshot(const Dictionary &p_args) {
	String viewport_path = p_args.get("viewport_path", "");
	String format = p_args.get("format", "png");

	SceneTree *st = mcp_get_scene_tree();
	if (!st) {
		return mcp_error("No SceneTree available");
	}

	Viewport *vp = nullptr;
	if (viewport_path.is_empty()) {
		vp = st->get_root();
	} else {
		Dictionary err;
		Node *node = mcp_get_node(viewport_path, err);
		if (!node) {
			return err;
		}
		vp = Object::cast_to<Viewport>(node);
		if (!vp) {
			return mcp_error("Node is not a Viewport: " + viewport_path);
		}
	}

	Ref<ViewportTexture> vp_tex = vp->get_texture();
	if (vp_tex.is_null()) {
		return mcp_error("Viewport texture is not available");
	}

	Ref<Image> img = vp_tex->get_image();
	if (img.is_null() || img->is_empty()) {
		return mcp_error("Failed to capture viewport image (image is empty or null)");
	}

	Vector<uint8_t> data;
	String mime_type;

	if (format == "jpg" || format == "jpeg") {
		data = img->save_jpg_to_buffer(0.85);
		mime_type = "image/jpeg";
	} else {
		data = img->save_png_to_buffer();
		mime_type = "image/png";
		format = "png";
	}

	if (data.is_empty()) {
		return mcp_error("Failed to encode image to " + format);
	}

	String base64 = CryptoCore::b64_encode_str(data.ptr(), data.size());

	Dictionary result;
	result["success"] = true;
	result["format"] = format;
	result["mime_type"] = mime_type;
	result["width"] = img->get_width();
	result["height"] = img->get_height();
	result["data_base64"] = base64;
	return result;
}

// --- get_node_screen_rect ---

static Dictionary mcp_tool_get_node_screen_rect(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Dictionary result;
	result["success"] = true;
	result["node_path"] = String(node->get_path());
	result["class"] = node->get_class();

	// Try 2D path: CanvasItem.
	CanvasItem *ci = Object::cast_to<CanvasItem>(node);
	if (ci) {
		Transform2D xform = ci->get_global_transform_with_canvas();
		Vector2 origin = xform.get_origin();

		Dictionary rect;
		rect["type"] = "2d";
		rect["origin_x"] = origin.x;
		rect["origin_y"] = origin.y;

		// For Control nodes, get the actual rect.
		Control *ctrl = Object::cast_to<Control>(node);
		if (ctrl) {
			Rect2 r = ctrl->get_global_rect();
			rect["x"] = r.position.x;
			rect["y"] = r.position.y;
			rect["width"] = r.size.x;
			rect["height"] = r.size.y;
		}

		result["screen_rect"] = rect;
		return result;
	}

	// Try 3D path: project AABB through camera.
	Node3D *n3d = Object::cast_to<Node3D>(node);
	if (n3d) {
		SceneTree *st = mcp_get_scene_tree();
		if (!st) {
			return mcp_error("No SceneTree available");
		}

		Viewport *vp = st->get_root();
		Camera3D *cam = vp->get_camera_3d();
		if (!cam) {
			return mcp_error("No active 3D camera found");
		}

		VisualInstance3D *vi = Object::cast_to<VisualInstance3D>(node);
		AABB aabb;
		if (vi) {
			aabb = vi->get_aabb();
		} else {
			// Use a small default AABB around the node origin.
			aabb = AABB(Vector3(-0.5, -0.5, -0.5), Vector3(1, 1, 1));
		}

		// Transform AABB to world space.
		Transform3D global_xform = n3d->get_global_transform();

		// Project all 8 corners of the AABB.
		Vector2 min_point = Vector2(1e10, 1e10);
		Vector2 max_point = Vector2(-1e10, -1e10);
		bool any_visible = false;

		for (int i = 0; i < 8; i++) {
			Vector3 corner;
			corner.x = (i & 1) ? aabb.position.x + aabb.size.x : aabb.position.x;
			corner.y = (i & 2) ? aabb.position.y + aabb.size.y : aabb.position.y;
			corner.z = (i & 4) ? aabb.position.z + aabb.size.z : aabb.position.z;

			Vector3 world_corner = global_xform.xform(corner);

			if (!cam->is_position_behind(world_corner)) {
				Vector2 screen_pos = cam->unproject_position(world_corner);
				min_point = min_point.min(screen_pos);
				max_point = max_point.max(screen_pos);
				any_visible = true;
			}
		}

		if (!any_visible) {
			return mcp_error("Node is entirely behind the camera");
		}

		Dictionary rect;
		rect["type"] = "3d_projected";
		rect["x"] = min_point.x;
		rect["y"] = min_point.y;
		rect["width"] = max_point.x - min_point.x;
		rect["height"] = max_point.y - min_point.y;
		result["screen_rect"] = rect;
		return result;
	}

	return mcp_error("Node is neither a CanvasItem (2D) nor a Node3D (3D): " + node_path);
}

// --- Registration ---

void mcp_register_screenshot_tools(MCPServer *p_server) {
	// take_viewport_screenshot
	{
		Array format_values;
		format_values.push_back("png");
		format_values.push_back("jpg");

		Dictionary schema = MCPSchema::object()
									.prop("viewport_path", "string", "Path to the Viewport node (empty for root viewport)")
									.prop_enum("format", format_values, "Image format for the screenshot (default: png)")
									.build();
		p_server->register_tool("take_viewport_screenshot",
				"Capture a viewport as a base64-encoded image",
				schema, callable_mp_static(&mcp_tool_take_viewport_screenshot));
	}

	// get_node_screen_rect
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the node in the scene tree", true)
									.build();
		p_server->register_tool("get_node_screen_rect",
				"Get the screen-space bounding rectangle of a 2D or 3D node",
				schema, callable_mp_static(&mcp_tool_get_node_screen_rect));
	}
}
