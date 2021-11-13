#pragma once
#include <glad/glad.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <EnumToString.h>

#include "VertexBuffer.h"
#include "IndexBuffer.h"

/// <summary>
/// We'll use this just to make it more clear what the intended usage of an attribute is in our code!
/// </summary>
ENUM(AttribUsage, uint8_t, 
	Unknown  = 0,
	Position = 1,
	Color    = 2,
	Color1   = 3,   //
	Color2   = 4,   // Extras
	Color3   = 5,   //
	Texture  = 6,
	Texture1 = 7, //
	Texture2 = 8, // Extras
	Texture3 = 9, //
	Normal   = 10,
	Tangent  = 11,
	BiNormal = 12,
	User0    = 13,    //
	User1    = 14,    //
	User2    = 15,    // Extras
	User3    = 16     //
);

/// <summary>
/// Represents the type that a VAO attribute can have
/// </summary>
/// <see>https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml</see>
ENUM(AttributeType, GLenum,
	Byte    = GL_BYTE,
	UByte   = GL_UNSIGNED_BYTE,
	Short   = GL_SHORT,
	UShort  = GL_UNSIGNED_SHORT,
	Int     = GL_INT,
	UInt    = GL_UNSIGNED_INT,
	Float   = GL_FLOAT,
	Double  = GL_DOUBLE,
	Unknown = GL_NONE
);

/// <summary>
/// Represents the mode in which a VAO will be drawn
/// </summary>
/// <see>https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArrays.xhtml</see>
ENUM(DrawMode, GLenum,
	Points        = GL_POINTS,
	LineStrip     = GL_LINE_STRIP,
	LineLoop      = GL_LINE_LOOP,
	LineList      = GL_LINES,
	TriangleStrip = GL_TRIANGLE_STRIP,
	TriangleFan   = GL_TRIANGLE_FAN,
	TriangleList  = GL_TRIANGLES
);

/// <summary>
/// This structure will represent the parameters passed to the glVertexAttribPointer commands
/// </summary>
struct BufferAttribute {
	/// <summary>
	/// The input slot to the vertex shader that will receive the data
	/// </summary>
	GLuint  Slot;
	/// <summary>
	/// The number of elements to be passed (ex 3 for a vec3)
	/// </summary>
	GLint   Size;
	/// <summary>
	/// The type of data to be passed (ex: GL_FLOAT for a vec3)
	/// </summary>
	AttributeType  Type;
	/// <summary>
	/// Specifies whether fixed-point data values should be normalized (true) or converted directly as fixed-point values (false) when they are accessed. Usually false
	/// </summary>
	bool    Normalized;
	/// <summary>
	/// The total size of an element in this buffer
	/// </summary>
	GLsizei Stride;
	/// <summary>
	/// The offset from the start of an element to this attribute
	/// </summary>
	GLsizei Offset;
	/// <summary>
	/// A hint for how the vertex attribute may be used (useful for our own code)
	/// </summary>
	AttribUsage Usage;

	BufferAttribute() :
		Slot(0), Size(0), Type(AttributeType::Unknown), Normalized(false), Stride(0), Offset(0), Usage(AttribUsage::Unknown){}

	BufferAttribute(uint32_t slot, uint32_t size, AttributeType type, GLsizei stride, GLsizei offset, AttribUsage usage, bool normalized = false) :
		Slot(slot), Size(size), Type(type), Stride(stride), Offset(offset), Usage(usage), Normalized(normalized) { }
};

/// <summary>
/// The Vertex Array Object wraps around an OpenGL VAO and basically represents all of the data for a mesh
/// </summary>
class VertexArrayObject final
{
public:
	typedef std::shared_ptr<VertexArrayObject> Sptr;
	typedef std::vector<BufferAttribute> VertexDeclaration;

	static inline Sptr Create() {
		return std::make_shared<VertexArrayObject>();
	}

	// We'll disallow moving and copying, since we want to manually control when the destructor is called
	// We'll use these classes via pointers
	VertexArrayObject(const VertexArrayObject& other) = delete;
	VertexArrayObject(VertexArrayObject&& other) = delete;
	VertexArrayObject& operator=(const VertexArrayObject& other) = delete;
	VertexArrayObject& operator=(VertexArrayObject&& other) = delete;

	// Helper structure to store a buffer and the attributes
	struct VertexBufferBinding {
		VertexBuffer::Sptr Buffer;
		std::vector<BufferAttribute> Attributes;
	};
	
public:
	/// <summary>
	/// Creates a new empty Vertex Array Object
	/// </summary>
	VertexArrayObject();
	// Destructor does not need to be virtual due to the use of the final keyword
	~VertexArrayObject();

	uint32_t GetVertexCount() const { return _vertexCount; }
	uint32_t GetIndexCount() const { return _indexBuffer != nullptr ? _indexBuffer->GetElementCount() : 0; }
	uint32_t GetElementCount() const { return _elementCount; }

	/// <summary>
	/// Sets the index buffer for this VAO, note that for now, this will not delete the buffer when the VAO is deleted, more on that later
	/// </summary>
	/// <param name="ibo">The index buffer to bind to this VAO</param>
	void SetIndexBuffer(const IndexBuffer::Sptr& ibo);
	IndexBuffer::Sptr GetIndexBuffer() const { return _indexBuffer; }
	/// <summary>
	/// Adds a vertex buffer to this VAO, with the specified attributes
	/// </summary>
	/// <param name="buffer">The buffer to add (note, does not take ownership, you will still need to delete later)</param>
	/// <param name="attributes">A list of vertex attributes that will be fed by this buffer</param>
	void AddVertexBuffer(const VertexBuffer::Sptr& buffer, const std::vector<BufferAttribute>& attributes);

	/// <summary>
	/// Gets the buffer binding that has an attribute with the given usage
	/// We can use this for extracting info from a VBO at a later time
	/// </summary>
	/// <param name="usage">The attribute usage hint to search for</param>
	/// <returns>A const pointer to the binding, or nullptr if none is found</returns>
	const VertexBufferBinding* GetBufferBinding(AttribUsage usage);

	void Draw(DrawMode mode = DrawMode::TriangleList);

	/// <summary>
	/// Binds this VAO as the source of data for draw operations
	/// </summary>
	void Bind();
	/// <summary>
	/// Unbinds the currently bound VAO
	/// </summary>
	static void Unbind();

	/// <summary>
	/// Returns the underlying OpenGL handle that this class is wrapping around
	/// </summary>
	GLuint GetHandle() const { return _handle; }

	void SetVDecl(const VertexDeclaration& vDecl);
	const VertexDeclaration& GetVDecl();

protected:
	
	// The index buffer bound to this VAO
	IndexBuffer::Sptr _indexBuffer;
	// The vertex buffers bound to this VAO
	std::vector<VertexBufferBinding> _vertexBuffers;

	// Stores a const pointer to one of the vertex declarations
	// defined in VertexTypes.cpp
	VertexDeclaration _vDecl;

	uint32_t _vertexCount;
	uint32_t _elementCount;

	// The underlying OpenGL handle that this class is wrapping around
	GLuint _handle;
};
