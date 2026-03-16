/**************************************************************************/
/*  mcp_tools_animation.cpp                                               */
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

#include "mcp_tools_animation.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/animation/animation_player.h"
#include "scene/animation/animation_tree.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"

// ---------------------------------------------------------------------------
// create_animation_player
// ---------------------------------------------------------------------------

static Dictionary _tool_create_animation_player(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "AnimationPlayer");

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	AnimationPlayer *player = memnew(AnimationPlayer);
	player->set_name(name);
	mcp_add_child_to_scene(parent, player);

	Dictionary result = mcp_success();
	result["node_path"] = String(player->get_path());
	return result;
}

// ---------------------------------------------------------------------------
// create_animation
// ---------------------------------------------------------------------------

static Dictionary _tool_create_animation(const Dictionary &p_args) {
	String player_path = p_args.get("player_path", "");
	String anim_name = p_args.get("anim_name", "");
	double length = p_args.get("length", 1.0);

	if (player_path.is_empty() || anim_name.is_empty()) {
		return mcp_error("player_path and anim_name are required");
	}

	Dictionary err;
	Node *node = mcp_get_node(player_path, err);
	if (!node) {
		return err;
	}

	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
	if (!player) {
		return mcp_error("Node is not an AnimationPlayer: " + player_path);
	}

	Ref<Animation> anim;
	anim.instantiate();
	anim->set_length(length);

	// Ensure the default animation library exists.
	if (!player->has_animation_library(StringName())) {
		Ref<AnimationLibrary> lib;
		lib.instantiate();
		player->add_animation_library(StringName(), lib);
	}

	Ref<AnimationLibrary> lib = player->get_animation_library(StringName());
	Error add_err = lib->add_animation(StringName(anim_name), anim);
	if (add_err != OK) {
		return mcp_error("Failed to add animation '" + anim_name + "' (error " + itos(add_err) + ")");
	}

	Dictionary result = mcp_success();
	result["animation_name"] = anim_name;
	result["length"] = length;
	return result;
}

// ---------------------------------------------------------------------------
// add_animation_track
// ---------------------------------------------------------------------------

static Dictionary _tool_add_animation_track(const Dictionary &p_args) {
	String player_path = p_args.get("player_path", "");
	String anim_name = p_args.get("anim_name", "");
	String node_path = p_args.get("node_path", "");
	String property = p_args.get("property", "");
	Array keyframes = p_args.get("keyframes", Array());

	if (player_path.is_empty() || anim_name.is_empty() || node_path.is_empty() || property.is_empty()) {
		return mcp_error("player_path, anim_name, node_path, and property are required");
	}

	Dictionary err;
	Node *node = mcp_get_node(player_path, err);
	if (!node) {
		return err;
	}

	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
	if (!player) {
		return mcp_error("Node is not an AnimationPlayer: " + player_path);
	}

	if (!player->has_animation(StringName(anim_name))) {
		return mcp_error("Animation not found: " + anim_name);
	}

	Ref<Animation> anim = player->get_animation(StringName(anim_name));

	int track_idx = anim->add_track(Animation::TYPE_VALUE);
	String track_path_str = node_path + ":" + property;
	anim->track_set_path(track_idx, NodePath(track_path_str));

	int keys_inserted = 0;
	for (int i = 0; i < keyframes.size(); i++) {
		Dictionary kf = keyframes[i];
		if (!kf.has("time") || !kf.has("value")) {
			continue;
		}
		double time = kf["time"];
		Variant value = kf["value"];
		anim->track_insert_key(track_idx, time, value);
		keys_inserted++;
	}

	Dictionary result = mcp_success();
	result["track_index"] = track_idx;
	result["track_path"] = track_path_str;
	result["keys_inserted"] = keys_inserted;
	return result;
}

// ---------------------------------------------------------------------------
// play_animation
// ---------------------------------------------------------------------------

static Dictionary _tool_play_animation(const Dictionary &p_args) {
	String player_path = p_args.get("player_path", "");
	String anim_name = p_args.get("anim_name", "");

	if (player_path.is_empty()) {
		return mcp_error("player_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(player_path, err);
	if (!node) {
		return err;
	}

	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
	if (!player) {
		return mcp_error("Node is not an AnimationPlayer: " + player_path);
	}

	if (!anim_name.is_empty() && !player->has_animation(StringName(anim_name))) {
		return mcp_error("Animation not found: " + anim_name);
	}

	player->play(StringName(anim_name));

	Dictionary result = mcp_success();
	result["playing"] = anim_name.is_empty() ? String("(default)") : anim_name;
	return result;
}

// ---------------------------------------------------------------------------
// stop_animation
// ---------------------------------------------------------------------------

static Dictionary _tool_stop_animation(const Dictionary &p_args) {
	String player_path = p_args.get("player_path", "");

	if (player_path.is_empty()) {
		return mcp_error("player_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(player_path, err);
	if (!node) {
		return err;
	}

	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
	if (!player) {
		return mcp_error("Node is not an AnimationPlayer: " + player_path);
	}

	player->stop();

	return mcp_success();
}

// ---------------------------------------------------------------------------
// get_animation_list
// ---------------------------------------------------------------------------

static Dictionary _tool_get_animation_list(const Dictionary &p_args) {
	String player_path = p_args.get("player_path", "");

	if (player_path.is_empty()) {
		return mcp_error("player_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(player_path, err);
	if (!node) {
		return err;
	}

	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
	if (!player) {
		return mcp_error("Node is not an AnimationPlayer: " + player_path);
	}

	LocalVector<StringName> anim_names;
	player->get_animation_list(&anim_names);

	Array animations;
	for (const StringName &anim_name : anim_names) {
		Dictionary anim_info;
		anim_info["name"] = String(anim_name);
		Ref<Animation> anim = player->get_animation(anim_name);
		if (anim.is_valid()) {
			anim_info["length"] = anim->get_length();
			anim_info["track_count"] = anim->get_track_count();
		}
		animations.push_back(anim_info);
	}

	Dictionary result = mcp_success();
	result["animations"] = animations;
	result["count"] = animations.size();
	return result;
}

// ---------------------------------------------------------------------------
// create_animation_tree
// ---------------------------------------------------------------------------

static Dictionary _tool_create_animation_tree(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "AnimationTree");
	String player_path = p_args.get("player_path", "");

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	AnimationTree *tree = memnew(AnimationTree);
	tree->set_name(name);
	mcp_add_child_to_scene(parent, tree);

	if (!player_path.is_empty()) {
		tree->set_animation_player(NodePath(player_path));
	}

	Dictionary result = mcp_success();
	result["node_path"] = String(tree->get_path());
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_animation_tools(MCPServer *p_server) {
	// create_animation_player
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the AnimationPlayer node (default: AnimationPlayer)")
									.build();

		p_server->register_tool("create_animation_player",
				"Create an AnimationPlayer node as a child of the specified parent",
				schema, callable_mp_static(_tool_create_animation_player));
	}

	// create_animation
	{
		Dictionary schema = MCPSchema::object()
									.prop("player_path", "string", "Path to the AnimationPlayer node", true)
									.prop("anim_name", "string", "Name of the animation to create", true)
									.prop_number("length", "Animation length in seconds (default: 1.0)")
									.build();

		p_server->register_tool("create_animation",
				"Create a new Animation resource in an AnimationPlayer",
				schema, callable_mp_static(_tool_create_animation));
	}

	// add_animation_track
	{
		Dictionary schema = MCPSchema::object()
									.prop("player_path", "string", "Path to the AnimationPlayer node", true)
									.prop("anim_name", "string", "Name of the animation to add the track to", true)
									.prop("node_path", "string", "Path to the target node (relative to AnimationPlayer root)", true)
									.prop("property", "string", "Property name to animate (e.g., position, modulate)", true)
									.prop_array("keyframes", "Array of keyframe objects with 'time' (number) and 'value' (variant) fields", true)
									.build();

		p_server->register_tool("add_animation_track",
				"Add a property track with keyframes to an animation",
				schema, callable_mp_static(_tool_add_animation_track));
	}

	// play_animation
	{
		Dictionary schema = MCPSchema::object()
									.prop("player_path", "string", "Path to the AnimationPlayer node", true)
									.prop("anim_name", "string", "Name of the animation to play (empty for default)")
									.build();

		p_server->register_tool("play_animation",
				"Play an animation on an AnimationPlayer",
				schema, callable_mp_static(_tool_play_animation));
	}

	// stop_animation
	{
		Dictionary schema = MCPSchema::object()
									.prop("player_path", "string", "Path to the AnimationPlayer node", true)
									.build();

		p_server->register_tool("stop_animation",
				"Stop the currently playing animation on an AnimationPlayer",
				schema, callable_mp_static(_tool_stop_animation));
	}

	// get_animation_list
	{
		Dictionary schema = MCPSchema::object()
									.prop("player_path", "string", "Path to the AnimationPlayer node", true)
									.build();

		p_server->register_tool("get_animation_list",
				"List all animations in an AnimationPlayer with their lengths and track counts",
				schema, callable_mp_static(_tool_get_animation_list));
	}

	// create_animation_tree
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the AnimationTree node (default: AnimationTree)")
									.prop("player_path", "string", "Path to the AnimationPlayer to link to")
									.build();

		p_server->register_tool("create_animation_tree",
				"Create an AnimationTree node linked to an AnimationPlayer",
				schema, callable_mp_static(_tool_create_animation_tree));
	}
}
