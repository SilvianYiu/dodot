/**************************************************************************/
/*  mcp_tools_tilemap.cpp                                                 */
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

#include "mcp_tools_tilemap.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/2d/tile_map_layer.h"
#include "scene/resources/2d/tile_set.h"

// ---------------------------------------------------------------------------
// create_tilemap
// ---------------------------------------------------------------------------

static Dictionary _tool_create_tilemap(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "TileMapLayer");
	int tile_size = p_args.get("tile_size", 16);

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	TileMapLayer *tilemap = memnew(TileMapLayer);
	tilemap->set_name(name);

	// Create a TileSet with the specified tile size.
	Ref<TileSet> tile_set;
	tile_set.instantiate();
	tile_set->set_tile_size(Vector2i(tile_size, tile_size));
	tilemap->set_tile_set(tile_set);

	mcp_add_child_to_scene(parent, tilemap);

	Dictionary result = mcp_success();
	result["node_path"] = String(tilemap->get_path());
	result["tile_size"] = tile_size;
	return result;
}

// ---------------------------------------------------------------------------
// set_tile
// ---------------------------------------------------------------------------

static Dictionary _tool_set_tile(const Dictionary &p_args) {
	String tilemap_path = p_args.get("tilemap_path", "");
	int coords_x = p_args.get("coords_x", 0);
	int coords_y = p_args.get("coords_y", 0);
	int source_id = p_args.get("source_id", 0);
	int atlas_x = p_args.get("atlas_coords_x", 0);
	int atlas_y = p_args.get("atlas_coords_y", 0);

	if (tilemap_path.is_empty()) {
		return mcp_error("tilemap_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(tilemap_path, err);
	if (!node) {
		return err;
	}

	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		return mcp_error("Node is not a TileMapLayer: " + tilemap_path);
	}

	tilemap->set_cell(Vector2i(coords_x, coords_y), source_id, Vector2i(atlas_x, atlas_y));

	Dictionary result = mcp_success();
	result["coords"] = Vector2i(coords_x, coords_y);
	return result;
}

// ---------------------------------------------------------------------------
// fill_rect
// ---------------------------------------------------------------------------

static Dictionary _tool_fill_rect(const Dictionary &p_args) {
	String tilemap_path = p_args.get("tilemap_path", "");
	int from_x = p_args.get("from_x", 0);
	int from_y = p_args.get("from_y", 0);
	int to_x = p_args.get("to_x", 0);
	int to_y = p_args.get("to_y", 0);
	int source_id = p_args.get("source_id", 0);
	int atlas_x = p_args.get("atlas_coords_x", 0);
	int atlas_y = p_args.get("atlas_coords_y", 0);

	if (tilemap_path.is_empty()) {
		return mcp_error("tilemap_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(tilemap_path, err);
	if (!node) {
		return err;
	}

	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		return mcp_error("Node is not a TileMapLayer: " + tilemap_path);
	}

	int min_x = MIN(from_x, to_x);
	int max_x = MAX(from_x, to_x);
	int min_y = MIN(from_y, to_y);
	int max_y = MAX(from_y, to_y);
	int tiles_set = 0;

	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			tilemap->set_cell(Vector2i(x, y), source_id, Vector2i(atlas_x, atlas_y));
			tiles_set++;
		}
	}

	Dictionary result = mcp_success();
	result["tiles_set"] = tiles_set;
	result["from"] = Vector2i(min_x, min_y);
	result["to"] = Vector2i(max_x, max_y);
	return result;
}

// ---------------------------------------------------------------------------
// clear_tilemap
// ---------------------------------------------------------------------------

static Dictionary _tool_clear_tilemap(const Dictionary &p_args) {
	String tilemap_path = p_args.get("tilemap_path", "");

	if (tilemap_path.is_empty()) {
		return mcp_error("tilemap_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(tilemap_path, err);
	if (!node) {
		return err;
	}

	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		return mcp_error("Node is not a TileMapLayer: " + tilemap_path);
	}

	tilemap->clear();

	return mcp_success();
}

// ---------------------------------------------------------------------------
// get_tilemap_info
// ---------------------------------------------------------------------------

static Dictionary _tool_get_tilemap_info(const Dictionary &p_args) {
	String tilemap_path = p_args.get("tilemap_path", "");

	if (tilemap_path.is_empty()) {
		return mcp_error("tilemap_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(tilemap_path, err);
	if (!node) {
		return err;
	}

	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		return mcp_error("Node is not a TileMapLayer: " + tilemap_path);
	}

	Dictionary result = mcp_success();
	result["node_path"] = String(tilemap->get_path());

	Ref<TileSet> tile_set = tilemap->get_tile_set();
	if (tile_set.is_valid()) {
		result["tile_size"] = tile_set->get_tile_size();
		result["tile_shape"] = tile_set->get_tile_shape();
		result["sources_count"] = tile_set->get_source_count();
	} else {
		result["tile_set"] = "none";
	}

	TypedArray<Vector2i> used_cells = tilemap->get_used_cells();
	result["used_cells_count"] = used_cells.size();

	Rect2i used_rect = tilemap->get_used_rect();
	Dictionary rect;
	rect["position_x"] = used_rect.position.x;
	rect["position_y"] = used_rect.position.y;
	rect["size_x"] = used_rect.size.x;
	rect["size_y"] = used_rect.size.y;
	result["used_rect"] = rect;

	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_tilemap_tools(MCPServer *p_server) {
	// create_tilemap
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the TileMapLayer node (default: TileMapLayer)")
									.prop_number("tile_size", "Tile size in pixels (default: 16)")
									.build();

		p_server->register_tool("create_tilemap",
				"Create a TileMapLayer node with a TileSet configured with the given tile size",
				schema, callable_mp_static(_tool_create_tilemap));
	}

	// set_tile
	{
		Dictionary schema = MCPSchema::object()
									.prop("tilemap_path", "string", "Path to the TileMapLayer node", true)
									.prop_number("coords_x", "Tile X coordinate in the grid", true)
									.prop_number("coords_y", "Tile Y coordinate in the grid", true)
									.prop_number("source_id", "TileSet source ID (default: 0)")
									.prop_number("atlas_coords_x", "Atlas X coordinate (default: 0)")
									.prop_number("atlas_coords_y", "Atlas Y coordinate (default: 0)")
									.build();

		p_server->register_tool("set_tile",
				"Set a tile at a specific grid position in a TileMapLayer",
				schema, callable_mp_static(_tool_set_tile));
	}

	// fill_rect
	{
		Dictionary schema = MCPSchema::object()
									.prop("tilemap_path", "string", "Path to the TileMapLayer node", true)
									.prop_number("from_x", "Start X coordinate", true)
									.prop_number("from_y", "Start Y coordinate", true)
									.prop_number("to_x", "End X coordinate", true)
									.prop_number("to_y", "End Y coordinate", true)
									.prop_number("source_id", "TileSet source ID (default: 0)")
									.prop_number("atlas_coords_x", "Atlas X coordinate (default: 0)")
									.prop_number("atlas_coords_y", "Atlas Y coordinate (default: 0)")
									.build();

		p_server->register_tool("fill_rect",
				"Fill a rectangular area with tiles in a TileMapLayer",
				schema, callable_mp_static(_tool_fill_rect));
	}

	// clear_tilemap
	{
		Dictionary schema = MCPSchema::object()
									.prop("tilemap_path", "string", "Path to the TileMapLayer node", true)
									.build();

		p_server->register_tool("clear_tilemap",
				"Clear all tiles from a TileMapLayer",
				schema, callable_mp_static(_tool_clear_tilemap));
	}

	// get_tilemap_info
	{
		Dictionary schema = MCPSchema::object()
									.prop("tilemap_path", "string", "Path to the TileMapLayer node", true)
									.build();

		p_server->register_tool("get_tilemap_info",
				"Get configuration and usage information about a TileMapLayer",
				schema, callable_mp_static(_tool_get_tilemap_info));
	}
}
