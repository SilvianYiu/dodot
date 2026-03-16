/**************************************************************************/
/*  mcp_tools_assets.cpp                                                  */
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

#include "mcp_tools_assets.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "scene/resources/image_texture.h"

// ---------------------------------------------------------------------------
// create_resource
// ---------------------------------------------------------------------------

static Dictionary _tool_create_resource(const Dictionary &p_args) {
	String type = p_args.get("type", "");
	String path = p_args.get("path", "");
	Dictionary properties = p_args.get("properties", Dictionary());

	if (type.is_empty() || path.is_empty()) {
		return mcp_error("type and path are required");
	}

	if (!ClassDB::class_exists(StringName(type))) {
		return mcp_error("Unknown resource type: " + type);
	}

	Object *obj = ClassDB::instantiate(StringName(type));
	if (!obj) {
		return mcp_error("Failed to instantiate resource type: " + type);
	}

	Ref<Resource> res = Object::cast_to<Resource>(obj);
	if (res.is_null()) {
		memdelete(obj);
		return mcp_error("Type is not a Resource: " + type);
	}

	// Apply properties.
	for (const Variant *key = properties.next(nullptr); key; key = properties.next(key)) {
		res->set(StringName(*key), properties[*key]);
	}

	Error save_err = ResourceSaver::save(res, path);
	if (save_err != OK) {
		return mcp_error("Failed to save resource to: " + path + " (error " + itos(save_err) + ")");
	}

	Dictionary result = mcp_success();
	result["type"] = type;
	result["path"] = path;
	return result;
}

// ---------------------------------------------------------------------------
// list_resources
// ---------------------------------------------------------------------------

static Dictionary _tool_list_resources(const Dictionary &p_args) {
	String directory = p_args.get("directory", "res://");
	String extension_filter = p_args.get("extension_filter", "");

	Ref<DirAccess> dir = DirAccess::open(directory);
	if (dir.is_null()) {
		return mcp_error("Cannot open directory: " + directory);
	}

	Array resources;
	dir->list_dir_begin();
	String file_name = dir->get_next();
	while (!file_name.is_empty()) {
		if (!dir->current_is_dir()) {
			if (extension_filter.is_empty() || file_name.get_extension() == extension_filter) {
				Dictionary info;
				info["name"] = file_name;
				info["path"] = directory.path_join(file_name);
				resources.push_back(info);
			}
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();

	Dictionary result = mcp_success();
	result["resources"] = resources;
	result["count"] = resources.size();
	result["directory"] = directory;
	return result;
}

// ---------------------------------------------------------------------------
// get_resource_info
// ---------------------------------------------------------------------------

static Dictionary _tool_get_resource_info(const Dictionary &p_args) {
	String path = p_args.get("path", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	if (!ResourceLoader::exists(path)) {
		return mcp_error("Resource not found: " + path);
	}

	Ref<Resource> res = ResourceLoader::load(path);
	if (res.is_null()) {
		return mcp_error("Failed to load resource: " + path);
	}

	Dictionary result = mcp_success();
	result["path"] = path;
	result["type"] = res->get_class();

	// Gather properties.
	Array props;
	List<PropertyInfo> prop_list;
	res->get_property_list(&prop_list);
	for (const PropertyInfo &pi : prop_list) {
		if (pi.usage & PROPERTY_USAGE_EDITOR) {
			Dictionary prop;
			prop["name"] = pi.name;
			prop["type"] = Variant::get_type_name(pi.type);
			prop["value"] = res->get(pi.name);
			props.push_back(prop);
		}
	}
	result["properties"] = props;

	return result;
}

// ---------------------------------------------------------------------------
// duplicate_resource
// ---------------------------------------------------------------------------

static Dictionary _tool_duplicate_resource(const Dictionary &p_args) {
	String source_path = p_args.get("source_path", "");
	String target_path = p_args.get("target_path", "");

	if (source_path.is_empty() || target_path.is_empty()) {
		return mcp_error("source_path and target_path are required");
	}

	if (!ResourceLoader::exists(source_path)) {
		return mcp_error("Source resource not found: " + source_path);
	}

	Ref<Resource> res = ResourceLoader::load(source_path);
	if (res.is_null()) {
		return mcp_error("Failed to load resource: " + source_path);
	}

	Ref<Resource> dup = res->duplicate();
	if (dup.is_null()) {
		return mcp_error("Failed to duplicate resource");
	}

	Error save_err = ResourceSaver::save(dup, target_path);
	if (save_err != OK) {
		return mcp_error("Failed to save duplicated resource to: " + target_path + " (error " + itos(save_err) + ")");
	}

	Dictionary result = mcp_success();
	result["source_path"] = source_path;
	result["target_path"] = target_path;
	result["type"] = dup->get_class();
	return result;
}

// ---------------------------------------------------------------------------
// generate_placeholder_texture
// ---------------------------------------------------------------------------

static Dictionary _tool_generate_placeholder_texture(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	int width = p_args.get("width", 64);
	int height = p_args.get("height", 64);
	String color_str = p_args.get("color", "ff0000ff");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	Color color = Color::html(color_str);

	Ref<Image> img = Image::create_empty(width, height, false, Image::FORMAT_RGBA8);
	img->fill(color);

	Error save_err = img->save_png(path);
	if (save_err != OK) {
		return mcp_error("Failed to save PNG to: " + path + " (error " + itos(save_err) + ")");
	}

	Dictionary result = mcp_success();
	result["path"] = path;
	result["width"] = width;
	result["height"] = height;
	result["color"] = color_str;
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_assets_tools(MCPServer *p_server) {
	// create_resource
	{
		Dictionary schema = MCPSchema::object()
									.prop("type", "string", "Godot resource class name (e.g., BoxMesh, AudioStreamWAV, Theme)", true)
									.prop("path", "string", "File path to save the resource (e.g., res://my_resource.tres)", true)
									.prop_object("properties", "Properties to set on the resource")
									.build();

		p_server->register_tool("create_resource",
				"Create any Godot Resource type and save it to disk",
				schema, callable_mp_static(_tool_create_resource));
	}

	// list_resources
	{
		Dictionary schema = MCPSchema::object()
									.prop("directory", "string", "Directory path to list (default: res://)", true)
									.prop("extension_filter", "string", "Filter by file extension (e.g., tres, tscn, png)")
									.build();

		p_server->register_tool("list_resources",
				"List resource files in a directory with optional extension filtering",
				schema, callable_mp_static(_tool_list_resources));
	}

	// get_resource_info
	{
		Dictionary schema = MCPSchema::object()
									.prop("path", "string", "Path to the resource file", true)
									.build();

		p_server->register_tool("get_resource_info",
				"Get type and editor-visible properties of a resource file",
				schema, callable_mp_static(_tool_get_resource_info));
	}

	// duplicate_resource
	{
		Dictionary schema = MCPSchema::object()
									.prop("source_path", "string", "Path to the source resource file", true)
									.prop("target_path", "string", "Path for the duplicated resource file", true)
									.build();

		p_server->register_tool("duplicate_resource",
				"Duplicate a resource file to a new path",
				schema, callable_mp_static(_tool_duplicate_resource));
	}

	// generate_placeholder_texture
	{
		Dictionary schema = MCPSchema::object()
									.prop("path", "string", "Output file path for the PNG (e.g., res://placeholder.png)", true)
									.prop_number("width", "Image width in pixels (default: 64)")
									.prop_number("height", "Image height in pixels (default: 64)")
									.prop("color", "string", "Fill color as HTML hex string (default: ff0000ff)")
									.build();

		p_server->register_tool("generate_placeholder_texture",
				"Generate a solid-color placeholder PNG texture",
				schema, callable_mp_static(_tool_generate_placeholder_texture));
	}
}
