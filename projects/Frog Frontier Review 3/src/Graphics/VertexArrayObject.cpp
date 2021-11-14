#include "VertexArrayObject.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Logging.h"

VertexArrayObject::VertexArrayObject() :
	_indexBuffer(nullptr),
	_handle(0),
	_vertexCount(0),
	_elementCount(0),
	_vertexBuffers(std::vector<VertexBufferBinding>())
{
	glCreateVertexArrays(1, &_handle);
}

VertexArrayObject::~VertexArrayObject()
{
	if (_handle != 0) {
		glDeleteVertexArrays(1, &_handle);
		_handle = 0;
	}
}

void VertexArrayObject::SetIndexBuffer(const IndexBuffer::Sptr& ibo) {
	// TODO: What if we already have a buffer? should we delete it? who owns the buffer?
	_indexBuffer = ibo;
	Bind();
	if (_indexBuffer != nullptr) {
		_indexBuffer->Bind();
		_elementCount = _indexBuffer->GetElementCount();
	}
	else {
		IndexBuffer::Unbind();
		_elementCount = _vertexCount;
	}
	Unbind();
}

void VertexArrayObject::AddVertexBuffer(const VertexBuffer::Sptr& buffer, const std::vector<BufferAttribute>& attributes) {
	if (_vertexBuffers.size() == 0) {
		_vertexCount = buffer->GetElementCount();
		if (_indexBuffer == nullptr) {
			_elementCount = _vertexCount;
		}
	} else if (buffer->GetElementCount() != _vertexCount) {
		LOG_WARN("Buffer element count does not match vertex count of this VAO!!!");
	}

	VertexBufferBinding binding;
	binding.Buffer = buffer;
	binding.Attributes = attributes;
	_vertexBuffers.push_back(binding);


	Bind();
	buffer->Bind();
	for (const BufferAttribute& attrib : attributes) {
		glEnableVertexArrayAttrib(_handle, attrib.Slot);
		glVertexAttribPointer(attrib.Slot, attrib.Size, (GLenum)attrib.Type, attrib.Normalized, attrib.Stride,
							  (void*)attrib.Offset);
	}
	Unbind();
}

void VertexArrayObject::Draw(DrawMode mode) {
	Bind();
	if (_indexBuffer == nullptr) {
		glDrawArrays((GLenum)mode, 0, _elementCount);
	} else {
		glDrawElements((GLenum)mode, _elementCount, (GLenum)_indexBuffer->GetElementType(), nullptr);
	}
	Unbind();
}

void VertexArrayObject::Bind() {
	glBindVertexArray(_handle);
}

void VertexArrayObject::Unbind() {
	glBindVertexArray(0);
}

void VertexArrayObject::SetVDecl(const VertexDeclaration& vDecl) {
	_vDecl = vDecl;
}

const VertexArrayObject::VertexDeclaration& VertexArrayObject::GetVDecl() {
	return _vDecl;
}

const VertexArrayObject::VertexBufferBinding* VertexArrayObject::GetBufferBinding(AttribUsage usage) {
	for (auto& binding : _vertexBuffers) {
		auto& it = std::find_if(binding.Attributes.begin(), binding.Attributes.end(), [&](const BufferAttribute& attrib) {
			return attrib.Usage == usage;
		});
		if (it != binding.Attributes.end()) {
			return &binding;
		}
	}
	return nullptr;
}
