#pragma once
#include <memory>
#include "Graphics/Texture2D.h"
#include "Graphics/Shader.h"

namespace Gameplay {
	/// <summary>
	/// Helper structure for material parameters to our shader
	/// THIS IS VERY TEMPORARY
	/// </summary>
	struct Material : public IResource {
	public:
		typedef std::shared_ptr<Material> Sptr;
		/// <summary>
		/// A human readable name for the material
		/// </summary>
		std::string     Name;
		/// <summary>
		/// The shader that the material is using
		/// </summary>
		Shader::Sptr    MatShader;

		/// <summary>
		/// Material shader parameters
		/// </summary>
		Texture2D::Sptr Texture;
		/// <summary>
		/// How reflective the material is, controls specular power
		/// </summary>
		float           Shininess;

		/// <summary>
		/// Handles applying this material's state to the OpenGL pipeline
		/// Will bind the shader, update material uniforms, and bind textures
		/// </summary>
		virtual void Apply();

		/// <summary>
		/// Loads a material from a JSON blob
		/// </summary>
		static Material::Sptr FromJson(const nlohmann::json& data);

		/// <summary>
		/// Converts this material into it's JSON representation for storage
		/// </summary>
		nlohmann::json ToJson() const;
	};
}