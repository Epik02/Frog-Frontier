#include "Gameplay/Material.h"
#include "Utils/ResourceManager/ResourceManager.h"

namespace Gameplay {
	void Material::Apply() {
		// Material properties
		MatShader->SetUniform("u_Material.Shininess", Shininess);

		// For textures, we pass the *slot* that the texture sure draw from
		MatShader->SetUniform("u_Material.Diffuse", 0);

		// Bind the texture
		if (Texture != nullptr) {
			Texture->Bind(0);
		}
	}

	Material::Sptr Material::FromJson(const nlohmann::json& data) {
		Material::Sptr result = std::make_shared<Material>();
		result->OverrideGUID(Guid(data["guid"]));
		result->Name = data["name"].get<std::string>();
		result->MatShader = ResourceManager::Get<Shader>(Guid(data["shader"]));

		// material specific parameters
		result->Texture = ResourceManager::Get<Texture2D>(Guid(data["texture"]));
		result->Shininess = data["shininess"].get<float>();
		return result;
	}

	nlohmann::json Material::ToJson() const { 
		return {
			{ "guid", GetGUID().str() },
			{ "name", Name },
			{ "shader", MatShader ? MatShader->GetGUID().str() : "null" },

			{ "texture", Texture ? Texture->IResource::GetGUID().str() : "null" },
			{ "shininess", Shininess },
		};
	}
}