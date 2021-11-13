#include "ObjLoader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <GLFW/glfw3.h>
#include <filesystem>

#include "Utils/StringUtils.h"

VertexArrayObject::Sptr ObjLoader::LoadFromFile(const std::string& filename)
{
	if (!std::filesystem::exists(filename)) {
		LOG_WARN("Failed to find OBJ file: \"{}\"", filename);
		return nullptr;
	}

	// Open our file in binary mode
	std::ifstream file;
	file.open(filename, std::ios::binary);

	// If our file fails to open, we will throw an error
	if (!file) {
		throw std::runtime_error("Failed to open file");
	}

	std::string line;
	
	// TODO: Load data from file
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<glm::ivec3> vertices;

	glm::vec3 vecData;
	glm::ivec3 vertexIndices;

	float startTime = glfwGetTime();

	// Read and process the entire file
	while (file.peek() != EOF) {
		// Read in the first part of the line (ex: f, v, vn, etc...)
		std::string command;
		file >> command;

		// We will ignore the rest of the line for comment lines
		if (command == "#") {
			std::getline(file, line);
		}

		// The v command defines a vertex's position
		else if (command == "v") {
			// Read in and store a position
			file >> vecData.x >> vecData.y >> vecData.z;
			positions.push_back(vecData);
		}
		// TODO: handle normals and textures
		else if (command == "vn") {
			// Read in and store a position
			file >> vecData.x >> vecData.y >> vecData.z;
			normals.push_back(vecData);
		} 
		else if (command == "vt") {
			// Read in and store a position
			file >> vecData.x >> vecData.y;
			uvs.push_back(vecData);
		}

		// The f command defines a polygon in the mesh
		// NOTE: make sure you triangulate in blender, otherwise it will
		// output quads instead of triangles
		else if (command == "f") {
			// Read the rest of the line from the file
			std::getline(file, line);
			// Trim whitespace from either end of the line
			StringTools::Trim(line);
			// Create a string stream so we can use streaming operators on it
			std::stringstream stream = std::stringstream(line);

			// We'll support only triangles
			for (int ix = 0; ix < 3; ix++) {
				// Read in the 3 attributes (position, UV, normal)
				char separator;
				stream >> vertexIndices.x >> separator >> vertexIndices.y >> separator >> vertexIndices.z;

				// The OBJ format can have negative values, which are a reference from the last added attributes
				if (vertexIndices.x < 0) { vertexIndices.x = positions.size() + 1 + vertexIndices.x; }
				if (vertexIndices.y < 0) { vertexIndices.y = uvs.size()       + 1 + vertexIndices.y; }
				if (vertexIndices.z < 0) { vertexIndices.z = normals.size()   + 1 + vertexIndices.z; }

				// OBJ format uses 1-based indices
				vertexIndices -= glm::ivec3(1);

				// add the vertex indices to the list
				// NOTE: This will create duplicate vertices!
				// A smarter solution would create a map of what attribute
				// combos have already been added
				vertices.push_back(vertexIndices);
			}
		}
	}

	// TODO: Generate mesh from the data we loaded
	std::vector<VertexPosNormTexCol> vertexData;

	for (int ix = 0; ix < vertices.size(); ix++) {
		glm::ivec3 attribs = vertices[ix];

		// Extract attributes from lists (except color)
		glm::vec3 position = positions[attribs.x];
		glm::vec2 uv       = uvs[attribs.y];
		glm::vec3 normal   = normals[attribs.z];
		glm::vec4 color    = glm::vec4(1.0f);

		// Add the vertex to the mesh
		vertexData.push_back(VertexPosNormTexCol(position, normal, uv, color));
	}

	// Create a vertex buffer and load all our vertex data
	VertexBuffer::Sptr vertexBuffer = VertexBuffer::Create();
	vertexBuffer->LoadData(vertexData.data(), vertexData.size());

	// Create the VAO, and add the vertices
	VertexArrayObject::Sptr result = VertexArrayObject::Create();
	result->AddVertexBuffer(vertexBuffer, VertexPosNormTexCol::V_DECL);

	result->SetVDecl(VertexPosNormTexCol::V_DECL);
	
	// Calculate and trace out how long it took us to load
	float endTime = glfwGetTime();
	LOG_TRACE("Loaded OBJ file \"{}\" in {} seconds ({} vertices, {} indices)", filename, endTime - startTime, vertexData.size(), 0);

	return result;
	//return VertexArrayObject::Create();
}
