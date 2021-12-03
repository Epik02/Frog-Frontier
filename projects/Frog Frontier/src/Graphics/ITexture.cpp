#include "ITexture.h"

ITexture::Limits ITexture::__limits = ITexture::Limits();
bool ITexture::__isStaticInit = false;

ITexture::ITexture(TextureType type) :
	_type(type),
	_handle(0)
{
	__StaticInit();
	_Recreate();
}

void ITexture::_Recreate()
{
	if (_handle == 0) {
		glDeleteTextures(1, &_handle);
	}
	glCreateTextures((GLenum)_type, 1, &_handle);
}

ITexture::~ITexture() {
	if (glIsTexture(_handle)) {
		glDeleteTextures(1, &_handle);
		_handle = 0;
	}
}

void ITexture::Bind(int slot) {
	if (_handle != 0) {
		// Instead of glActiveTexture + glBindTexture, we can one line it now :D
		glBindTextureUnit(slot, _handle); 
	}
}

void ITexture::Unbind(int slot) {
	glBindTextureUnit(slot, 0);
}

void ITexture::Clear(const glm::vec4& color) {
	if (_handle != 0) {
		glClearTexImage(_handle, 0, GL_RGBA, GL_FLOAT, &color.x);
	}
}

void ITexture::__StaticInit()
{
	// If we've already run the static initializer, abort now
	if (__isStaticInit) return;

	// Example of reading limits from the OpenGL renderer
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &__limits.MAX_TEXTURE_SIZE);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &__limits.MAX_TEXTURE_UNITS);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &__limits.MAX_3D_TEXTURE_SIZE);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &__limits.MAX_TEXTURE_IMAGE_UNITS);
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &__limits.MAX_ANISOTROPY);

	// Enable seamless cube maps (we'll need this later!)
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// Let's write all our info into the console so we know what's up
	LOG_INFO("==== Texture Limits =====");
	LOG_INFO("\tSize:       {}", __limits.MAX_TEXTURE_SIZE);
	LOG_INFO("\tUnits:      {}", __limits.MAX_TEXTURE_UNITS);
	LOG_INFO("\t3D Size:    {}", __limits.MAX_3D_TEXTURE_SIZE);
	LOG_INFO("\tUnits (FS): {}", __limits.MAX_TEXTURE_IMAGE_UNITS);
	LOG_INFO("\tMax Aniso.: {}", __limits.MAX_ANISOTROPY);

	__isStaticInit = true;
}

ITexture::Limits ITexture::GetLimits() {
	__StaticInit();
	return __limits;
}
