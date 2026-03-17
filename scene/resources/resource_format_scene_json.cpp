/**************************************************************************/
/*  resource_format_scene_json.cpp                                        */
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

#include "resource_format_scene_json.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "scene/resources/packed_scene.h"

// =============================================================================
// Variant <-> JSON Conversion
// =============================================================================

Variant SceneJSONVariant::variant_to_json(const Variant &p_variant, HashMap<String, Ref<Resource>> &r_ext_resources, HashMap<String, Ref<Resource>> &r_sub_resources, int &r_sub_id) {
	switch (p_variant.get_type()) {
		case Variant::NIL:
			return Variant();
		case Variant::BOOL:
			return p_variant;
		case Variant::INT:
			return p_variant;
		case Variant::FLOAT:
			return p_variant;
		case Variant::STRING:
			return p_variant;
		case Variant::STRING_NAME: {
			Dictionary d;
			d["_type"] = "StringName";
			d["value"] = String(p_variant);
			return d;
		}
		case Variant::VECTOR2: {
			Vector2 v = p_variant;
			Dictionary d;
			d["_type"] = "Vector2";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR2I: {
			Vector2i v = p_variant;
			Dictionary d;
			d["_type"] = "Vector2i";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR3: {
			Vector3 v = p_variant;
			Dictionary d;
			d["_type"] = "Vector3";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::VECTOR3I: {
			Vector3i v = p_variant;
			Dictionary d;
			d["_type"] = "Vector3i";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::VECTOR4: {
			Vector4 v = p_variant;
			Dictionary d;
			d["_type"] = "Vector4";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			d["w"] = v.w;
			return d;
		}
		case Variant::VECTOR4I: {
			Vector4i v = p_variant;
			Dictionary d;
			d["_type"] = "Vector4i";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			d["w"] = v.w;
			return d;
		}
		case Variant::RECT2: {
			Rect2 r = p_variant;
			Dictionary d;
			d["_type"] = "Rect2";
			d["x"] = r.position.x;
			d["y"] = r.position.y;
			d["w"] = r.size.x;
			d["h"] = r.size.y;
			return d;
		}
		case Variant::RECT2I: {
			Rect2i r = p_variant;
			Dictionary d;
			d["_type"] = "Rect2i";
			d["x"] = r.position.x;
			d["y"] = r.position.y;
			d["w"] = r.size.x;
			d["h"] = r.size.y;
			return d;
		}
		case Variant::COLOR: {
			Color c = p_variant;
			Dictionary d;
			d["_type"] = "Color";
			d["r"] = c.r;
			d["g"] = c.g;
			d["b"] = c.b;
			d["a"] = c.a;
			return d;
		}
		case Variant::TRANSFORM2D: {
			Transform2D t = p_variant;
			Dictionary d;
			d["_type"] = "Transform2D";
			Array cols;
			for (int i = 0; i < 3; i++) {
				Dictionary col;
				col["x"] = t.columns[i].x;
				col["y"] = t.columns[i].y;
				cols.push_back(col);
			}
			d["columns"] = cols;
			return d;
		}
		case Variant::BASIS: {
			Basis b = p_variant;
			Dictionary d;
			d["_type"] = "Basis";
			Array rows;
			for (int i = 0; i < 3; i++) {
				Dictionary row;
				row["x"] = b.rows[i].x;
				row["y"] = b.rows[i].y;
				row["z"] = b.rows[i].z;
				rows.push_back(row);
			}
			d["rows"] = rows;
			return d;
		}
		case Variant::TRANSFORM3D: {
			Transform3D t = p_variant;
			Dictionary d;
			d["_type"] = "Transform3D";
			Dictionary basis;
			Array rows;
			for (int i = 0; i < 3; i++) {
				Dictionary row;
				row["x"] = t.basis.rows[i].x;
				row["y"] = t.basis.rows[i].y;
				row["z"] = t.basis.rows[i].z;
				rows.push_back(row);
			}
			basis["rows"] = rows;
			d["basis"] = basis;
			Dictionary origin;
			origin["x"] = t.origin.x;
			origin["y"] = t.origin.y;
			origin["z"] = t.origin.z;
			d["origin"] = origin;
			return d;
		}
		case Variant::QUATERNION: {
			Quaternion q = p_variant;
			Dictionary d;
			d["_type"] = "Quaternion";
			d["x"] = q.x;
			d["y"] = q.y;
			d["z"] = q.z;
			d["w"] = q.w;
			return d;
		}
		case Variant::AABB: {
			AABB a = p_variant;
			Dictionary d;
			d["_type"] = "AABB";
			d["px"] = a.position.x;
			d["py"] = a.position.y;
			d["pz"] = a.position.z;
			d["sx"] = a.size.x;
			d["sy"] = a.size.y;
			d["sz"] = a.size.z;
			return d;
		}
		case Variant::PLANE: {
			Plane p = p_variant;
			Dictionary d;
			d["_type"] = "Plane";
			d["x"] = p.normal.x;
			d["y"] = p.normal.y;
			d["z"] = p.normal.z;
			d["d"] = p.d;
			return d;
		}
		case Variant::PROJECTION: {
			Projection proj = p_variant;
			Dictionary d;
			d["_type"] = "Projection";
			Array cols;
			for (int i = 0; i < 4; i++) {
				Dictionary col;
				col["x"] = proj.columns[i].x;
				col["y"] = proj.columns[i].y;
				col["z"] = proj.columns[i].z;
				col["w"] = proj.columns[i].w;
				cols.push_back(col);
			}
			d["columns"] = cols;
			return d;
		}
		case Variant::NODE_PATH: {
			Dictionary d;
			d["_type"] = "NodePath";
			d["path"] = String(NodePath(p_variant));
			return d;
		}
		case Variant::RID: {
			Dictionary d;
			d["_type"] = "RID";
			d["id"] = (int64_t)RID(p_variant).get_id();
			return d;
		}
		case Variant::OBJECT: {
			Object *obj = p_variant;
			if (!obj) {
				return Variant();
			}
			Ref<Resource> res = p_variant;
			if (res.is_valid()) {
				String path = res->get_path();
				if (!path.is_empty() && !path.contains("::")) {
					// External resource.
					String id;
					bool found = false;
					for (const KeyValue<String, Ref<Resource>> &kv : r_ext_resources) {
						if (kv.value == res) {
							id = kv.key;
							found = true;
							break;
						}
					}
					if (!found) {
						id = "ext_" + itos(r_ext_resources.size() + 1);
						r_ext_resources[id] = res;
					}
					Dictionary d;
					d["_type"] = "ExtResource";
					d["id"] = id;
					return d;
				} else {
					// Sub-resource (embedded).
					String id;
					bool found = false;
					for (const KeyValue<String, Ref<Resource>> &kv : r_sub_resources) {
						if (kv.value == res) {
							id = kv.key;
							found = true;
							break;
						}
					}
					if (!found) {
						id = "sub_" + itos(++r_sub_id);
						r_sub_resources[id] = res;
					}
					Dictionary d;
					d["_type"] = "SubResource";
					d["id"] = id;
					return d;
				}
			}
			// Non-resource object — store as string representation.
			Dictionary d;
			d["_type"] = "Object";
			d["class"] = obj->get_class();
			return d;
		}
		case Variant::DICTIONARY: {
			Dictionary src = p_variant;
			Dictionary dst;
			for (const Variant *key = src.next(nullptr); key; key = src.next(key)) {
				String key_str = String(*key);
				dst[key_str] = variant_to_json(src[*key], r_ext_resources, r_sub_resources, r_sub_id);
			}
			// Mark as a plain dictionary to distinguish from typed objects.
			Dictionary wrapper;
			wrapper["_type"] = "Dictionary";
			wrapper["data"] = dst;
			return wrapper;
		}
		case Variant::ARRAY: {
			Array src = p_variant;
			Array dst;
			for (int i = 0; i < src.size(); i++) {
				dst.push_back(variant_to_json(src[i], r_ext_resources, r_sub_resources, r_sub_id));
			}
			return dst;
		}
		case Variant::PACKED_BYTE_ARRAY: {
			PackedByteArray arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedByteArray";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_INT32_ARRAY: {
			PackedInt32Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedInt32Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_INT64_ARRAY: {
			PackedInt64Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedInt64Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_FLOAT32_ARRAY: {
			PackedFloat32Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedFloat32Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_FLOAT64_ARRAY: {
			PackedFloat64Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedFloat64Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_STRING_ARRAY: {
			PackedStringArray arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedStringArray";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				vals.push_back(arr[i]);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_VECTOR2_ARRAY: {
			PackedVector2Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedVector2Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				Dictionary v;
				v["x"] = arr[i].x;
				v["y"] = arr[i].y;
				vals.push_back(v);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_VECTOR3_ARRAY: {
			PackedVector3Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedVector3Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				Dictionary v;
				v["x"] = arr[i].x;
				v["y"] = arr[i].y;
				v["z"] = arr[i].z;
				vals.push_back(v);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_COLOR_ARRAY: {
			PackedColorArray arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedColorArray";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				Dictionary c;
				c["r"] = arr[i].r;
				c["g"] = arr[i].g;
				c["b"] = arr[i].b;
				c["a"] = arr[i].a;
				vals.push_back(c);
			}
			d["data"] = vals;
			return d;
		}
		case Variant::PACKED_VECTOR4_ARRAY: {
			PackedVector4Array arr = p_variant;
			Dictionary d;
			d["_type"] = "PackedVector4Array";
			Array vals;
			for (int i = 0; i < arr.size(); i++) {
				Dictionary v;
				v["x"] = arr[i].x;
				v["y"] = arr[i].y;
				v["z"] = arr[i].z;
				v["w"] = arr[i].w;
				vals.push_back(v);
			}
			d["data"] = vals;
			return d;
		}
		default: {
			// Fallback: use string representation.
			Dictionary d;
			d["_type"] = "Unknown";
			d["variant_type"] = Variant::get_type_name(p_variant.get_type());
			d["value"] = p_variant.stringify();
			return d;
		}
	}
}

Variant SceneJSONVariant::json_to_variant(const Variant &p_json, const HashMap<String, Ref<Resource>> &p_resources) {
	if (p_json.get_type() == Variant::NIL) {
		return Variant();
	}

	if (p_json.get_type() == Variant::BOOL || p_json.get_type() == Variant::INT || p_json.get_type() == Variant::FLOAT || p_json.get_type() == Variant::STRING) {
		return p_json;
	}

	if (p_json.get_type() == Variant::ARRAY) {
		Array src = p_json;
		Array dst;
		for (int i = 0; i < src.size(); i++) {
			dst.push_back(json_to_variant(src[i], p_resources));
		}
		return dst;
	}

	if (p_json.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_json;
		if (!d.has("_type")) {
			// Plain dictionary — convert values recursively.
			Dictionary dst;
			for (const Variant *key = d.next(nullptr); key; key = d.next(key)) {
				dst[*key] = json_to_variant(d[*key], p_resources);
			}
			return dst;
		}

		String type = d["_type"];

		// Helper lambda to safely read a numeric field from a dictionary.
#define SCENE_JSON_GET_NUM(dict, key) ((dict).has((key)) ? (double)(dict)[(key)] : 0.0)

		if (type == "StringName") {
			return StringName(String(d.get("value", "")));
		} else if (type == "Vector2") {
			return Vector2(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"));
		} else if (type == "Vector2i") {
			return Vector2i(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"));
		} else if (type == "Vector3") {
			return Vector3(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z"));
		} else if (type == "Vector3i") {
			return Vector3i(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z"));
		} else if (type == "Vector4") {
			return Vector4(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z"), SCENE_JSON_GET_NUM(d, "w"));
		} else if (type == "Vector4i") {
			return Vector4i(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z"), SCENE_JSON_GET_NUM(d, "w"));
		} else if (type == "Rect2") {
			return Rect2(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "w"), SCENE_JSON_GET_NUM(d, "h"));
		} else if (type == "Rect2i") {
			return Rect2i(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "w"), SCENE_JSON_GET_NUM(d, "h"));
		} else if (type == "Color") {
			return Color(SCENE_JSON_GET_NUM(d, "r"), SCENE_JSON_GET_NUM(d, "g"), SCENE_JSON_GET_NUM(d, "b"), SCENE_JSON_GET_NUM(d, "a"));
		} else if (type == "Transform2D") {
			Transform2D t;
			if (!d.has("columns") || d["columns"].get_type() != Variant::ARRAY) {
				WARN_PRINT("Malformed Transform2D: missing 'columns' array.");
				return t;
			}
			Array cols = d["columns"];
			for (int i = 0; i < 3 && i < cols.size(); i++) {
				if (cols[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary col = cols[i];
				t.columns[i] = Vector2(SCENE_JSON_GET_NUM(col, "x"), SCENE_JSON_GET_NUM(col, "y"));
			}
			return t;
		} else if (type == "Basis") {
			Basis b;
			if (!d.has("rows") || d["rows"].get_type() != Variant::ARRAY) {
				WARN_PRINT("Malformed Basis: missing 'rows' array.");
				return b;
			}
			Array rows = d["rows"];
			for (int i = 0; i < 3 && i < rows.size(); i++) {
				if (rows[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary row = rows[i];
				b.rows[i] = Vector3(SCENE_JSON_GET_NUM(row, "x"), SCENE_JSON_GET_NUM(row, "y"), SCENE_JSON_GET_NUM(row, "z"));
			}
			return b;
		} else if (type == "Transform3D") {
			Transform3D t;
			if (!d.has("basis") || d["basis"].get_type() != Variant::DICTIONARY) {
				WARN_PRINT("Malformed Transform3D: missing 'basis' dictionary.");
				return t;
			}
			Dictionary basis = d["basis"];
			if (!basis.has("rows") || basis["rows"].get_type() != Variant::ARRAY) {
				WARN_PRINT("Malformed Transform3D: missing 'basis.rows' array.");
				return t;
			}
			Array rows = basis["rows"];
			for (int i = 0; i < 3 && i < rows.size(); i++) {
				if (rows[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary row = rows[i];
				t.basis.rows[i] = Vector3(SCENE_JSON_GET_NUM(row, "x"), SCENE_JSON_GET_NUM(row, "y"), SCENE_JSON_GET_NUM(row, "z"));
			}
			if (d.has("origin") && d["origin"].get_type() == Variant::DICTIONARY) {
				Dictionary origin = d["origin"];
				t.origin = Vector3(SCENE_JSON_GET_NUM(origin, "x"), SCENE_JSON_GET_NUM(origin, "y"), SCENE_JSON_GET_NUM(origin, "z"));
			}
			return t;
		} else if (type == "Quaternion") {
			return Quaternion(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z"), SCENE_JSON_GET_NUM(d, "w"));
		} else if (type == "AABB") {
			return AABB(Vector3(SCENE_JSON_GET_NUM(d, "px"), SCENE_JSON_GET_NUM(d, "py"), SCENE_JSON_GET_NUM(d, "pz")), Vector3(SCENE_JSON_GET_NUM(d, "sx"), SCENE_JSON_GET_NUM(d, "sy"), SCENE_JSON_GET_NUM(d, "sz")));
		} else if (type == "Plane") {
			return Plane(Vector3(SCENE_JSON_GET_NUM(d, "x"), SCENE_JSON_GET_NUM(d, "y"), SCENE_JSON_GET_NUM(d, "z")), (real_t)SCENE_JSON_GET_NUM(d, "d"));
		} else if (type == "Projection") {
			Projection proj;
			if (!d.has("columns") || d["columns"].get_type() != Variant::ARRAY) {
				WARN_PRINT("Malformed Projection: missing 'columns' array.");
				return proj;
			}
			Array cols = d["columns"];
			for (int i = 0; i < 4 && i < cols.size(); i++) {
				if (cols[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary col = cols[i];
				proj.columns[i] = Vector4(SCENE_JSON_GET_NUM(col, "x"), SCENE_JSON_GET_NUM(col, "y"), SCENE_JSON_GET_NUM(col, "z"), SCENE_JSON_GET_NUM(col, "w"));
			}
			return proj;
		} else if (type == "NodePath") {
			return NodePath(String(d.get("path", "")));
		} else if (type == "RID") {
			return RID(); // RIDs are runtime-only, cannot restore.
		} else if (type == "ExtResource" || type == "SubResource") {
			String id = d["id"];
			if (p_resources.has(id)) {
				return p_resources[id];
			}
			return Variant();
		} else if (type == "Dictionary") {
			Dictionary src = d["data"];
			Dictionary dst;
			for (const Variant *key = src.next(nullptr); key; key = src.next(key)) {
				dst[*key] = json_to_variant(src[*key], p_resources);
			}
			return dst;
		} else if (type == "PackedByteArray") {
			Array vals = d["data"];
			PackedByteArray arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedInt32Array") {
			Array vals = d["data"];
			PackedInt32Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedInt64Array") {
			Array vals = d["data"];
			PackedInt64Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedFloat32Array") {
			Array vals = d["data"];
			PackedFloat32Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedFloat64Array") {
			Array vals = d["data"];
			PackedFloat64Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedStringArray") {
			Array vals = d["data"];
			PackedStringArray arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				arr.set(i, vals[i]);
			}
			return arr;
		} else if (type == "PackedVector2Array") {
			Array vals = d.get("data", Array());
			PackedVector2Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				if (vals[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary v = vals[i];
				arr.set(i, Vector2(SCENE_JSON_GET_NUM(v, "x"), SCENE_JSON_GET_NUM(v, "y")));
			}
			return arr;
		} else if (type == "PackedVector3Array") {
			Array vals = d.get("data", Array());
			PackedVector3Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				if (vals[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary v = vals[i];
				arr.set(i, Vector3(SCENE_JSON_GET_NUM(v, "x"), SCENE_JSON_GET_NUM(v, "y"), SCENE_JSON_GET_NUM(v, "z")));
			}
			return arr;
		} else if (type == "PackedColorArray") {
			Array vals = d.get("data", Array());
			PackedColorArray arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				if (vals[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary c = vals[i];
				arr.set(i, Color(SCENE_JSON_GET_NUM(c, "r"), SCENE_JSON_GET_NUM(c, "g"), SCENE_JSON_GET_NUM(c, "b"), SCENE_JSON_GET_NUM(c, "a")));
			}
			return arr;
		} else if (type == "PackedVector4Array") {
			Array vals = d.get("data", Array());
			PackedVector4Array arr;
			arr.resize(vals.size());
			for (int i = 0; i < vals.size(); i++) {
				if (vals[i].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary v = vals[i];
				arr.set(i, Vector4(SCENE_JSON_GET_NUM(v, "x"), SCENE_JSON_GET_NUM(v, "y"), SCENE_JSON_GET_NUM(v, "z"), SCENE_JSON_GET_NUM(v, "w")));
			}
			return arr;
		}

		// Unknown typed dictionary — return as-is.
		return d;
	}

#undef SCENE_JSON_GET_NUM

	return p_json;
}

// =============================================================================
// Helper: serialize a sub-resource to JSON
// =============================================================================

static Dictionary _serialize_sub_resource(const Ref<Resource> &p_res, HashMap<String, Ref<Resource>> &r_ext_resources, HashMap<String, Ref<Resource>> &r_sub_resources, int &r_sub_id) {
	Dictionary d;
	d["type"] = p_res->get_class();

	List<PropertyInfo> props;
	p_res->get_property_list(&props);

	Dictionary properties;
	for (const PropertyInfo &pi : props) {
		if (pi.usage & PROPERTY_USAGE_STORAGE) {
			Variant val = p_res->get(pi.name);
			if (val.get_type() == Variant::NIL) {
				continue; // Skip null values.
			}
			properties[pi.name] = SceneJSONVariant::variant_to_json(val, r_ext_resources, r_sub_resources, r_sub_id);
		}
	}
	if (!properties.is_empty()) {
		d["properties"] = properties;
	}
	return d;
}

// =============================================================================
// Saver
// =============================================================================

Error ResourceFormatSaverSceneJSON::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<PackedScene> scene = p_resource;
	ERR_FAIL_COND_V(scene.is_null(), ERR_INVALID_PARAMETER);

	Ref<SceneState> state = scene->get_state();
	ERR_FAIL_COND_V(state.is_null(), ERR_INVALID_DATA);

	// Collect resources from all node properties.
	HashMap<String, Ref<Resource>> ext_resources;
	HashMap<String, Ref<Resource>> sub_resources;
	int sub_id = 0;

	// First pass: collect all resources.
	for (int i = 0; i < state->get_node_count(); i++) {
		for (int j = 0; j < state->get_node_property_count(i); j++) {
			Variant val = state->get_node_property_value(i, j);
			// variant_to_json will populate ext/sub resource maps.
			SceneJSONVariant::variant_to_json(val, ext_resources, sub_resources, sub_id);
		}
		// Instance (sub-scene) reference.
		Ref<PackedScene> instance = state->get_node_instance(i);
		if (instance.is_valid()) {
			String id;
			bool found = false;
			for (const KeyValue<String, Ref<Resource>> &kv : ext_resources) {
				if (kv.value == instance) {
					id = kv.key;
					found = true;
					break;
				}
			}
			if (!found) {
				id = "ext_" + itos(ext_resources.size() + 1);
				ext_resources[id] = instance;
			}
		}
	}

	// Build JSON structure.
	Dictionary scene_json;
	scene_json["format_version"] = 1;
	scene_json["type"] = "PackedScene";

	// External resources.
	if (!ext_resources.is_empty()) {
		Array ext_arr;
		for (const KeyValue<String, Ref<Resource>> &kv : ext_resources) {
			Dictionary rd;
			rd["id"] = kv.key;
			rd["type"] = kv.value->get_class();
			rd["path"] = kv.value->get_path();
			ext_arr.push_back(rd);
		}
		scene_json["external_resources"] = ext_arr;
	}

	// Sub-resources.
	if (!sub_resources.is_empty()) {
		Array sub_arr;
		for (const KeyValue<String, Ref<Resource>> &kv : sub_resources) {
			Dictionary rd = _serialize_sub_resource(kv.value, ext_resources, sub_resources, sub_id);
			rd["id"] = kv.key;
			sub_arr.push_back(rd);
		}
		scene_json["sub_resources"] = sub_arr;
	}

	// Nodes.
	Array nodes;
	for (int i = 0; i < state->get_node_count(); i++) {
		Dictionary node;
		node["name"] = String(state->get_node_name(i));
		node["type"] = String(state->get_node_type(i));

		if (i > 0) {
			node["parent"] = String(state->get_node_path(i, true));
		}

		// Instance reference.
		Ref<PackedScene> instance = state->get_node_instance(i);
		if (instance.is_valid()) {
			for (const KeyValue<String, Ref<Resource>> &kv : ext_resources) {
				if (kv.value == instance) {
					node["instance"] = kv.key;
					break;
				}
			}
		}

		// Properties.
		Dictionary props;
		for (int j = 0; j < state->get_node_property_count(i); j++) {
			String pname = state->get_node_property_name(i, j);
			Variant pval = state->get_node_property_value(i, j);
			props[pname] = SceneJSONVariant::variant_to_json(pval, ext_resources, sub_resources, sub_id);
		}
		if (!props.is_empty()) {
			node["properties"] = props;
		}

		// Groups.
		Vector<StringName> groups = state->get_node_groups(i);
		if (!groups.is_empty()) {
			Array grp;
			for (int g = 0; g < groups.size(); g++) {
				grp.push_back(String(groups[g]));
			}
			node["groups"] = grp;
		}

		nodes.push_back(node);
	}
	scene_json["nodes"] = nodes;

	// Connections.
	if (state->get_connection_count() > 0) {
		Array scene_connections;
		for (int i = 0; i < state->get_connection_count(); i++) {
			Dictionary conn;
			conn["signal"] = String(state->get_connection_signal(i));
			conn["from"] = String(state->get_connection_source(i));
			conn["to"] = String(state->get_connection_target(i));
			conn["method"] = String(state->get_connection_method(i));
			int flags = state->get_connection_flags(i);
			if (flags != 0) {
				conn["flags"] = flags;
			}
			int unbinds = state->get_connection_unbinds(i);
			if (unbinds > 0) {
				conn["unbinds"] = unbinds;
			}
			Array binds = state->get_connection_binds(i);
			if (!binds.is_empty()) {
				Array json_binds;
				for (int b = 0; b < binds.size(); b++) {
					json_binds.push_back(SceneJSONVariant::variant_to_json(binds[b], ext_resources, sub_resources, sub_id));
				}
				conn["binds"] = json_binds;
			}
			scene_connections.push_back(conn);
		}
		scene_json["connections"] = scene_connections;
	}

	// Editable instances.
	Vector<NodePath> editable_instances = state->get_editable_instances();
	if (!editable_instances.is_empty()) {
		Array editable;
		for (int i = 0; i < editable_instances.size(); i++) {
			editable.push_back(String(editable_instances[i]));
		}
		scene_json["editable_instances"] = editable;
	}

	// Write file.
	String json_str = JSON::stringify(scene_json, "\t", false, true);

	Error err;
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Cannot save scene JSON '%s'.", p_path));

	fa->store_string(json_str);
	fa->store_8('\n');

	return fa->get_error() != OK ? ERR_CANT_CREATE : OK;
}

void ResourceFormatSaverSceneJSON::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	Ref<PackedScene> scene = p_resource;
	if (scene.is_valid()) {
		p_extensions->push_back("scene.json");
	}
}

bool ResourceFormatSaverSceneJSON::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<PackedScene>(*p_resource) != nullptr;
}

bool ResourceFormatSaverSceneJSON::recognize_path(const Ref<Resource> &p_resource, const String &p_path) const {
	return p_path.ends_with(".scene.json");
}

// =============================================================================
// Loader
// =============================================================================

Ref<Resource> ResourceFormatLoaderSceneJSON::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	if (!FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		return Ref<Resource>();
	}

	String json_str = FileAccess::get_file_as_string(p_path);
	if (json_str.is_empty()) {
		if (r_error) {
			*r_error = ERR_FILE_CORRUPT;
		}
		return Ref<Resource>();
	}

	Ref<JSON> json;
	json.instantiate();
	Error parse_err = json->parse(json_str);
	if (parse_err != OK || json->get_data().get_type() != Variant::DICTIONARY) {
		ERR_PRINT(vformat("Failed to parse scene JSON '%s': %s at line %d.", p_path, json->get_error_message(), json->get_error_line()));
		if (r_error) {
			*r_error = ERR_FILE_CORRUPT;
		}
		return Ref<Resource>();
	}

	Dictionary scene_data = json->get_data();

	// Verify format.
	int version = scene_data.get("format_version", 0);
	if (version != 1) {
		ERR_PRINT(vformat("Unsupported scene JSON format version %d in '%s'.", version, p_path));
		if (r_error) {
			*r_error = ERR_FILE_UNRECOGNIZED;
		}
		return Ref<Resource>();
	}

	// Load external resources.
	HashMap<String, Ref<Resource>> all_resources;
	Array ext_resources = scene_data.get("external_resources", Array());
	for (int i = 0; i < ext_resources.size(); i++) {
		Dictionary rd = ext_resources[i];
		String id = rd.get("id", "");
		String path = rd.get("path", "");
		if (!path.is_empty()) {
			Ref<Resource> res = ResourceLoader::load(path);
			if (res.is_valid()) {
				all_resources[id] = res;
			} else {
				ERR_PRINT(vformat("Failed to load external resource '%s' (id: %s) in '%s'.", path, id, p_path));
				if (r_error) {
					*r_error = ERR_FILE_MISSING_DEPENDENCIES;
				}
				return Ref<Resource>();
			}
		}
	}

	// Create sub-resources.
	Array sub_resources = scene_data.get("sub_resources", Array());
	for (int i = 0; i < sub_resources.size(); i++) {
		Dictionary rd = sub_resources[i];
		String id = rd.get("id", "");
		String type = rd.get("type", "");

		if (type.is_empty() || id.is_empty()) {
			continue;
		}

		Ref<Resource> res;
		if (ClassDB::can_instantiate(type)) {
			Object *obj = ClassDB::instantiate(type);
			if (obj) {
				Resource *r = Object::cast_to<Resource>(obj);
				if (r) {
					res = Ref<Resource>(r);
				} else {
					memdelete(obj);
				}
			}
		}

		if (res.is_valid()) {
			all_resources[id] = res;

			// Set properties.
			Dictionary props = rd.get("properties", Dictionary());
			for (const Variant *key = props.next(nullptr); key; key = props.next(key)) {
				String prop_name = String(*key);
				Variant val = SceneJSONVariant::json_to_variant(props[*key], all_resources);
				res->set(prop_name, val);
			}
		} else {
			WARN_PRINT(vformat("Cannot instantiate sub-resource type '%s' (id: %s) in '%s'.", type, id, p_path));
		}
	}

	// Build SceneState.
	Ref<SceneState> state;
	state.instantiate();

	Array nodes = scene_data.get("nodes", Array());

	// Map from node index to SceneState node index.
	HashMap<int, int> node_index_map;
	// Map from path string to node index for parent resolution.
	HashMap<String, int> path_to_index;

	for (int i = 0; i < nodes.size(); i++) {
		Dictionary node = nodes[i];
		String name = node.get("name", "Node");
		String type = node.get("type", "Node");
		String parent_path = node.get("parent", "");

		int name_idx = state->add_name(name);
		int type_idx = state->add_name(type);

		int parent_idx = -1;
		if (i == 0) {
			parent_idx = -1; // Root node.
		} else {
			// Resolve parent path via node_path.
			NodePath np(parent_path);
			PackedInt32Array uid_path;
			parent_idx = state->add_node_path(np, uid_path);
		}

		int owner_idx = -1;
		if (i > 0) {
			// Owner is the root node (index 0).
			owner_idx = 0;
		}

		int instance_idx = -1;
		if (node.has("instance")) {
			String inst_id = node["instance"];
			if (all_resources.has(inst_id)) {
				instance_idx = state->add_value(all_resources[inst_id]);
			}
		}

		int node_idx = state->add_node(parent_idx, owner_idx, type_idx, name_idx, instance_idx, -1, -1);
		node_index_map[i] = node_idx;

		// Build path for this node.
		if (i == 0) {
			path_to_index["."] = node_idx;
		}

		// Properties.
		Dictionary props = node.get("properties", Dictionary());
		for (const Variant *key = props.next(nullptr); key; key = props.next(key)) {
			String prop_name = String(*key);
			Variant val = SceneJSONVariant::json_to_variant(props[*key], all_resources);
			int pname_idx = state->add_name(prop_name);
			int pval_idx = state->add_value(val);
			bool is_deferred = (val.get_type() == Variant::NODE_PATH);
			state->add_node_property(node_idx, pname_idx, pval_idx, is_deferred);
		}

		// Groups.
		Array groups = node.get("groups", Array());
		for (int g = 0; g < groups.size(); g++) {
			int grp_idx = state->add_name(groups[g]);
			state->add_node_group(node_idx, grp_idx);
		}
	}

	// Connections.
	Array scene_connections = scene_data.get("connections", Array());
	for (int i = 0; i < scene_connections.size(); i++) {
		Dictionary conn = scene_connections[i];
		NodePath from_path(String(conn.get("from", "")));
		NodePath to_path(String(conn.get("to", "")));
		String signal_name = conn.get("signal", "");
		String method_name = conn.get("method", "");
		int flags = conn.get("flags", 0);
		int unbinds = conn.get("unbinds", 0);

		PackedInt32Array uid_path;
		int from_idx = state->add_node_path(from_path, uid_path);
		int to_idx = state->add_node_path(to_path, uid_path);
		int signal_idx = state->add_name(signal_name);
		int method_idx = state->add_name(method_name);

		Vector<int> bind_indices;
		Array binds = conn.get("binds", Array());
		for (int b = 0; b < binds.size(); b++) {
			Variant val = SceneJSONVariant::json_to_variant(binds[b], all_resources);
			bind_indices.push_back(state->add_value(val));
		}

		state->add_connection(from_idx, to_idx, signal_idx, method_idx, flags, unbinds, bind_indices);
	}

	// Editable instances.
	Array editable = scene_data.get("editable_instances", Array());
	for (int i = 0; i < editable.size(); i++) {
		state->add_editable_instance(NodePath(String(editable[i])));
	}

	// Pack into PackedScene.
	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	packed_scene->set_path(p_path);

	// Use the state we built.
	packed_scene->get_state()->copy_from(state);

	if (r_error) {
		*r_error = OK;
	}

	return packed_scene;
}

void ResourceFormatLoaderSceneJSON::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("scene.json");
}

bool ResourceFormatLoaderSceneJSON::recognize_path(const String &p_path, const String &p_for_type) const {
	return p_path.ends_with(".scene.json");
}

bool ResourceFormatLoaderSceneJSON::handles_type(const String &p_type) const {
	return p_type == "PackedScene";
}

String ResourceFormatLoaderSceneJSON::get_resource_type(const String &p_path) const {
	if (p_path.ends_with(".scene.json")) {
		return "PackedScene";
	}
	return "";
}
