#pragma once
#include <memory>
#include <glad/glad.h>
#include <cstdint>
#include <Graphics/TextureEnums.h>
#include <GLM/glm.hpp>
#include "Utils/ResourceManager/IResource.h"

/// <summary>
/// The abstract base class for all our textures that we'll be implementing
/// </summary>
class ITexture : public IResource
{
public:
	typedef std::shared_ptr<ITexture> Sptr;

	// Remove the copy and and assignment operators
	ITexture(const ITexture& other) = delete;
	ITexture(ITexture&& other) = delete;
	ITexture& operator=(const ITexture& other) = delete;
	ITexture& operator=(ITexture&& other) = delete;
	
	/// <summary>
	/// Represents the limits available to OpenGL textures on the current renderer
	/// </summary>
	struct Limits {
		int   MAX_TEXTURE_SIZE;
		int   MAX_TEXTURE_UNITS;
		int   MAX_3D_TEXTURE_SIZE;
		int   MAX_TEXTURE_IMAGE_UNITS;
		float MAX_ANISOTROPY;
	};
	
	/// <summary>
	/// Virtual destructor that cleans up texture
	/// </summary>
	virtual ~ITexture();

	/// <summary>
	/// Binds this texture to the given texture slot
	/// </summary>
	/// <param name="slot">The slot to bind, 0 &lt;= slot &lt; MAX_TEXTURE_UNITS</param>
	virtual void Bind(int slot);
	/// <summary>
	/// Unbinds all textures from the given texture slot
	/// </summary>
	/// <param name="slot">The slot to unbind, 0 &lt;= slot &lt; MAX_TEXTURE_UNITS</param>
	static void Unbind(int slot);

	/// <summary>
	/// Clears the first level of this texture to a solid color, note this only works for color texture types!
	/// </summary>
	/// <param name="color">The color to clear to</param>
	void Clear(const glm::vec4& color);

protected:
	ITexture(TextureType type);

	/// <summary>
	/// Recreates the texture, for instance when we want to resize an image
	/// </summary>
	virtual void _Recreate();

	GLuint _handle;    // The OpenGL handle for this textureW
	TextureType _type; // The type for this texture, mainly used for debugging

// STATIC SECTION
private:
	static Limits __limits;
	static bool __isStaticInit;

	static void __StaticInit();

public:
	/// <summary>
	/// Gets the driver texture limits for the current renderer
	/// </summary>
	/// <returns>All fetched texture limits for the current renderer</returns>
	static Limits GetLimits();
};

