#pragma once
#include <GLM/glm.hpp>
#include "json.hpp"
#include "GLM/gtc/quaternion.hpp"

inline glm::vec2 ParseJsonVec2(const nlohmann::json& blob) {
	glm::vec2 result;
	result.x = blob["x"].get<float>();
	result.y = blob["y"].get<float>();
	return result;
}
inline glm::vec3 ParseJsonVec3(const nlohmann::json& blob) {
	glm::vec3 result;
	result.x = blob["x"].get<float>();
	result.y = blob["y"].get<float>();
	result.z = blob["z"].get<float>();
	return result;
}
inline glm::vec4 ParseJsonVec4(const nlohmann::json& blob) {
	glm::vec4 result;
	result.x = blob["x"].get<float>();
	result.y = blob["y"].get<float>();
	result.z = blob["z"].get<float>();
	result.w = blob["w"].get<float>();
	return result;
}
inline glm::quat ParseJsonQuat(const nlohmann::json& blob) {
	glm::quat result;
	result.x = blob["x"].get<float>();
	result.y = blob["y"].get<float>();
	result.z = blob["z"].get<float>();
	result.w = blob["w"].get<float>();
	return result;
}

inline nlohmann::json GlmToJson(const glm::vec2& value) {
	return {
		{"x", value.x},
		{"y", value.y}
	};
}
inline nlohmann::json GlmToJson(const glm::vec3& value) {
	return {
		{"x", value.x},
		{"y", value.y},
		{"z", value.z}
	};
}
inline nlohmann::json GlmToJson(const glm::vec4& value) {
	return {
		{"x", value.x},
		{"y", value.y},
		{"z", value.z},
		{"w", value.w}
	};
}
inline nlohmann::json GlmToJson(const glm::quat& value) {
	return {
		{"x", value.x},
		{"y", value.y},
		{"z", value.z},
		{"w", value.w}
	};
}

template <typename T>
T JsonGet(const nlohmann::json& blob, const std::string& key, const T& defaultRet = T()) {
	if (blob.find(key) != blob.end()) {
		return blob[key].get<T>();
	} else {
		return defaultRet;
	}
}

#define JsonParseEnum(typeName, blob, key, defaultVal) (blob.contains(key) ? Parse##typeName(blob[key].get<std::string>(), defaultVal) : defaultVal)