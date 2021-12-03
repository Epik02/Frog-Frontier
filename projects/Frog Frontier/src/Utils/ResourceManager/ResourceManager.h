#pragma once

#include <json.hpp>
#include <unordered_map>
#include <typeindex>

#include "Graphics/Texture2D.h";
#include "Graphics/VertexArrayObject.h";
#include "Graphics/Shader.h";
#include "Gameplay/Material.h";

#include "Utils/GUID.hpp"
#include "Utils/ResourceManager/IResource.h"

/// <summary>
/// Utility class for managing and loading resources from JSON
/// manifest files
/// </summary>
class ResourceManager {
public:
	/// <summary>
	/// Initializes the resource manager and performs any first-time
	/// setup required
	/// </summary>
	static void Init();

	/// <summary>
	/// Creates a new asset, and forwards the arguments to it's constructor
	/// </summary>
	/// <typeparam name="T">The type of asset to create</typeparam>
	/// <typeparam name="...TArgs">The types for the arguments to forward to the constructor</typeparam>
	/// <param name="...args">The arguments to forward to the constructor</param>
	/// <returns>The GUID of the newly created asset</returns>
	template <typename T, typename ... TArgs, typename = std::enable_if<is_valid_resource<T>()>::type>
	static std::shared_ptr<T> CreateAsset(TArgs&&... args) {
		// Create and store the asset
		std::shared_ptr<T> asset = std::make_shared<T>(std::forward<TArgs>(args)...);
		_resources[std::type_index(typeid(T))][asset->IResource::GetGUID()] = asset;

		// Get the JSON representation of the asset so we can store it in the manifest
		nlohmann::json data = asset->ToJson();

		// Make sure the data has the GUID
		std::string guid = asset->IResource::GetGUID().str();
		data["guid"] = guid;

		// Store the JSON data in the resource manifest (based on the type's name)
		_manifest[StringTools::SanitizeClassName(typeid(T).name())][guid] = data;
		return asset;
	}

	/// <summary>
	/// Gets a shared pointer to the resource with the given type and GUID
	/// </summary>
	/// <typeparam name="T">The type of resource to retreive</typeparam>
	/// <param name="id">The ID of the resource to retrieve</param>
	/// <returns>The resource with the given GUID, or nullptr if none exists</returns>
	template<typename T, typename = std::enable_if<is_valid_resource<T>()>::type>
	static std::shared_ptr<T> Get(Guid id) {
		return std::dynamic_pointer_cast<T>(_resources[std::type_index(typeid(T))][id]);
	}

	/// <summary>
	/// Registers a resource type with the resource manager, only types that have been registered
	/// can be loaded from JSON manifest files!
	/// </summary>
	/// <typeparam name="T">The type to register, must satisfy the is_valid_resource constraint</typeparam>
	/// <typeparam name=""></typeparam>
	template <typename T, typename = std::enable_if<is_valid_resource<T>()>::type>
	static void RegisterType() {
		// Extract the type name from a sanitized version of they typeid name
		std::string typeName = StringTools::SanitizeClassName(typeid(T).name());

		// Create the type loader for the type
		_typeLoaders[typeName] = [](const nlohmann::json& data) {
			IResource::Sptr res = T::FromJson(data);
			res->OverrideGUID(Guid(data["guid"]));
			_resources[std::type_index(typeid(T))][res->GetGUID()] = res;
			return res->GetGUID();
		};

		// Make sure we haven't registered the type yet, then add an empty object
		// to the manifest to ensure it can be saved
		if (!_manifest.contains(typeName)) {
			_manifest[typeName] = nlohmann::json();
		}
	}

	/// <summary>
	/// Gets the current JSON manifest
	/// </summary>
	static const nlohmann::json& GetManifest();
	/// <summary>
	/// Loads a manifest file into the resource manager
	/// </summary>
	/// <param name="path">The path to the JSON manifest file</param>
	static void LoadManifest(const std::string& path);
	/// <summary>
	/// Saves the manifest to the given JSON file
	/// </summary>
	/// <param name="path">The path to the file to output</param>
	static void SaveManifest(const std::string& path);

	/// <summary>
	/// Releases all resources held by the resource manager
	/// </summary>
	static void Cleanup();

protected:
	/// <summary>
	/// This is a map of maps
	/// The top level map uses type_index, so there's a map per resource type
	/// The inner map handles mapping GUIDs to the corresponding resource
	/// </summary>
	static std::map<std::type_index, std::map<Guid, IResource::Sptr>> _resources;
	/// <summary>
	/// This map stores registered types, so we can load them from JSON files
	/// </summary>
	static std::map<std::string, std::function<Guid(const nlohmann::json&)>> _typeLoaders;

	static nlohmann::json _manifest;
};