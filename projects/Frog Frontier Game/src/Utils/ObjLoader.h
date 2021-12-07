#pragma once

#include "MeshBuilder.h"
#include "MeshFactory.h"

class ObjLoader
{
public:
	static VertexArrayObject::Sptr LoadFromFile(const std::string& filename);

protected:
	ObjLoader() = default;
	~ObjLoader() = default;
};