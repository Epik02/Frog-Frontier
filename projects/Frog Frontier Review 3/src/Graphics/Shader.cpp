#include "Shader.h"
#include "Logging.h"
#include <fstream>
#include <sstream>

Shader::Shader() :
	// We zero out all of our members so we don't have garbage data in our class
	_vs(0),
	_fs(0),
	_handle(0)
{
	_handle = glCreateProgram();
}

Shader::Shader(const std::unordered_map<ShaderPartType, std::string>& filePaths) :
	IResource(),
	_handle(0)
{
	_handle = glCreateProgram();
	for (auto& [type, path] : filePaths) {
		LoadShaderPartFromFile(path.c_str(), type);
	}
	Link();
}


Shader::~Shader() {
	if (_handle != 0) {
		glDeleteProgram(_handle);
		_handle = 0;
	}
}

bool Shader::LoadShaderPart(const char* source, ShaderPartType type)
{
	// Creates a new shader part (VS, FS, GS, etc...)
	GLuint handle = glCreateShader((GLenum)type);

	// Load the GLSL source and compile it
	glShaderSource(handle, 1, &source, nullptr);
	glCompileShader(handle);

	// Get the compilation status for the shader part
	GLint status = 0;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		// Get the size of the error log
		GLint logSize = 0;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logSize);

		// Create a new character buffer for the log
		char* log = new char[logSize];

		// Get the log
		glGetShaderInfoLog(handle, logSize, &logSize, log);

		// Dump error log
		LOG_ERROR("Failed to compile shader part:\n{}", log);

		// Clean up our log memory
		delete[] log;

		// Delete the broken shader result
		glDeleteShader(handle);
		handle = 0;

		return false;
	}

	switch (type) {
		case ShaderPartType::Vertex: _vs = handle; break;
		case ShaderPartType::Fragment: _fs = handle; break;
		default: LOG_WARN("Not implemented"); break;
	}

	_fileSourceMap[type].Source = source;
	_fileSourceMap[type].IsFilePath = false;

	return status != GL_FALSE;
}

bool Shader::LoadShaderPartFromFile(const char* path, ShaderPartType type) {
	// Open the file at path
	std::ifstream file(path);

	// Check to see if the file is open
	if (file.is_open()) {
		// Create a string stream to load the entire contents of the file
		std::stringstream stream;
		stream << file.rdbuf();

		// Compile the shader part from the loaded contents of the file
		bool result = LoadShaderPart(stream.str().c_str(), type);

		_fileSourceMap[type].Source = path;
		_fileSourceMap[type].IsFilePath = true;

		// Close the file
		file.close();
		return result;
	}
	// Failed to open file, log it and return false
	else {
		LOG_WARN("Could not open file at \"{}\"", path);
		return false;
	}
}

bool Shader::Link()
{
	LOG_ASSERT(_vs != 0 && _fs != 0, "Must attach both a vertex and fragment shader!");

	// Attach our two shaders
	glAttachShader(_handle, _vs);
	glAttachShader(_handle, _fs);

	// Perform linking
	glLinkProgram(_handle);

	// Remove shader parts to save space (we can do this since we only needed the shader parts to compile an actual shader program)
	glDetachShader(_handle, _vs);
	glDeleteShader(_vs);
	glDetachShader(_handle, _fs);
	glDeleteShader(_fs);

	GLint status = 0;
	glGetProgramiv(_handle, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		// Get the length of the log
		GLint length = 0;
		glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &length);

		if (length > 0) {
			// Read the log from openGL
			char* log = new char[length];
			glGetProgramInfoLog(_handle, length, &length, log);
			LOG_ERROR("Shader failed to link:\n{}", log);
			delete[] log;
		} else {
			LOG_ERROR("Shader failed to link for an unknown reason!");
		}
	}
	return status != GL_FALSE;
}

void Shader::Bind() {
	// Simply calls glUseProgram with our shader handle
	glUseProgram(_handle);
}

void Shader::Unbind() {
	// We unbind a shader program by using the default program (0)
	glUseProgram(0);
}

void Shader::SetUniformMatrix(int location, const glm::mat3* value, int count, bool transposed) {
	glProgramUniformMatrix3fv(_handle, location, count, transposed, glm::value_ptr(*value));
}
void Shader::SetUniformMatrix(int location, const glm::mat4* value, int count, bool transposed) {
	glProgramUniformMatrix4fv(_handle, location, count, transposed, glm::value_ptr(*value));
}

void Shader::SetUniform(int location, const float* value, int count) {
	glProgramUniform1fv(_handle, location, count, value);
}
void Shader::SetUniform(int location, const glm::vec2* value, int count) {
	glProgramUniform2fv(_handle, location, count, glm::value_ptr(*value));
}
void Shader::SetUniform(int location, const glm::vec3* value, int count) {
	glProgramUniform3fv(_handle, location, count, glm::value_ptr(*value));
}
void Shader::SetUniform(int location, const glm::vec4* value, int count) {
	glProgramUniform4fv(_handle, location, count, glm::value_ptr(*value));
}

void Shader::SetUniform(int location, const int* value, int count) {
	glProgramUniform1iv(_handle, location, count, value);
}
void Shader::SetUniform(int location, const glm::ivec2* value, int count) {
	glProgramUniform2iv(_handle, location, count, glm::value_ptr(*value));
}
void Shader::SetUniform(int location, const glm::ivec3* value, int count) {
	glProgramUniform3iv(_handle, location, count, glm::value_ptr(*value));
}
void Shader::SetUniform(int location, const glm::ivec4* value, int count) {
	glProgramUniform4iv(_handle, location, count, glm::value_ptr(*value));
}

void Shader::SetUniform(int location, const bool* value, int count) {
	LOG_ASSERT(count == 1, "SetUniform for bools only supports setting single values at a time!");
	glProgramUniform1i(location, *value, 1);
}
void Shader::SetUniform(int location, const glm::bvec2* value, int count) {
	LOG_ASSERT(count == 1, "SetUniform for bools only supports setting single values at a time!");
	glProgramUniform2i(location, value->x, value->y, 1);
}
void Shader::SetUniform(int location, const glm::bvec3* value, int count) {
	LOG_ASSERT(count == 1, "SetUniform for bools only supports setting single values at a time!");
	glProgramUniform3i(location, value->x, value->y, value->z, 1);
}
void Shader::SetUniform(int location, const glm::bvec4* value, int count) {
	LOG_ASSERT(count == 1, "SetUniform for bools only supports setting single values at a time!");
	glProgramUniform4i(location, value->x, value->y, value->z, value->w, 1);
}

int Shader::__GetUniformLocation(const std::string& name) {
	// Search the map for the given name
	std::unordered_map<std::string, int>::const_iterator it = _uniformLocs.find(name);
	int result = -1;

	// If our entry was not found, we call glGetUniform and store it for next time
	if (it == _uniformLocs.end()) {
		result = glGetUniformLocation(_handle, name.c_str());
		_uniformLocs[name] = result;
	}
	// Otherwise, we had a value in the map, return it
	else {
		result = it->second;
	}

	return result;
}


nlohmann::json Shader::ToJson() const {
	nlohmann::json result;
	for (auto& [key, value] : _fileSourceMap) {
		result[~key][value.IsFilePath ? "path" : "source"] = value.Source;
	}
	return result;

}

Shader::Sptr Shader::FromJson(const nlohmann::json& data) {
	Shader::Sptr result = std::make_shared<Shader>();
	for (auto& [key, blob] : data.items()) {
		// Get the shader part type from the key
		ShaderPartType type = ParseShaderPartType(key, ShaderPartType::Unknown);
		// As long as the type is valid
		if (type != ShaderPartType::Unknown) {
			// If it has a file, we load from file
			if (blob.contains("file")) {
				result->LoadShaderPartFromFile(blob["file"].get<std::string>().c_str(), type);
			}
			// Otherwise we see if there's a source and load that instead
			else if (blob.contains("source")) {
				result->LoadShaderPart(blob["source"].get<std::string>().c_str(), type);
			}
			// Otherwise do nothing
		}
	}
	result->Link();
	return result;
}