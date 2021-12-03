﻿/*
* This file contains general purpose OpenGL enums that may be used in various places throughout our game engine
* The major feature is ShaderDataType, which allows us to gather information about GL Uniforms and their data
* structure
*/
#pragma once
#include <EnumToString.h>
#include <glad/glad.h>
#include <Logging.h>
#include <glm/glm.hpp>

/*
	* Represents the type of data used in a shader in a more useful format for us
	* than what OpenGL provides to us
	*
	* The bitwise makeup of these values is shown below:
	*
	* HIGH              12            6       3      LOW 
	* ┌───┬─────────────┬─────────────┬───────┬───────┐
	* │ 0 │ 0 0 0 0 0 0 │ 0 0 0 0 0 0 │ 0 0 0 │ 0 0 0 │
	* ├─┬─┼─────────────┼─────────────┼───────┼───────┤
	* │ │ │  Underlying │   RESERVED  │Column │  Row  │ 
	* │ │ │  Data Type  │ FOR TEXTURES│Length │Length │
 	* │ │ └─────────────┴─────────────┴───────┴───────┤
	* │ └ Set to 1 for texture types                  │
	* └───────────────────────────────────────────────┘
	*/
#pragma region ShaderDataType
ENUM(ShaderDataType, uint32_t,
		None = 0,

		Float   = 0b00000001'000000'000'001,
		Float2  = 0b00000001'000000'000'010,
		Float3  = 0b00000001'000000'000'011,
		Float4  = 0b00000001'000000'000'100,
		Mat2    = 0b00000010'000000'010'010,
		Mat3    = 0b00000010'000000'011'011,
		Mat4    = 0b00000010'000000'100'100,
		Mat2x3  = 0b00000010'000000'010'011,
		Mat2x4  = 0b00000010'000000'010'100,
		Mat3x2  = 0b00000010'000000'011'010,
		Mat3x4  = 0b00000010'000000'011'100,
		Mat4x2  = 0b00000010'000000'100'010,
		Mat4x3  = 0b00000010'000000'100'011,

		Int     = 0b00000100'000000'000'001,
		Int2    = 0b00000100'000000'000'010,
		Int3    = 0b00000100'000000'000'011,
		Int4    = 0b00000100'000000'000'100,

		Uint    = 0b00001000'000000'000'001,
		Uint2   = 0b00001000'000000'000'010,
		Uint3   = 0b00001000'000000'000'011,
		Uint4   = 0b00001000'000000'000'100,

		Uint64  = 0b00001000'000000'001'001,

		Double  = 0b00010000'000000'000'001,
		Double2 = 0b00010000'000000'000'010,
		Double3 = 0b00010000'000000'000'011,
		Double4 = 0b00010000'000000'000'100,

		// Note: Double precision matrices do require some newer RenderAPI's (mainly OpenGL4 or DX11 level hardware)

		Dmat2   = 0b00100000'00000'010'0010,
		Dmat3   = 0b00100000'00000'011'0011,
		Dmat4   = 0b00100000'00000'100'0100,
		Dmat2x3 = 0b00100000'00000'010'0011,
		Dmat2x4 = 0b00100000'00000'010'0100,
		Dmat3x2 = 0b00100000'00000'011'0010,
		Dmat3x4 = 0b00100000'00000'011'0100,
		Dmat4x2 = 0b00100000'00000'100'0010,
		Dmat4x3 = 0b00100000'00000'100'0011,

		Bool    = 0b01000000'00000'000'0001,
		Bool2   = 0b01000000'00000'000'0010,
		Bool3   = 0b01000000'00000'000'0011,
		Bool4   = 0b01000000'00000'000'0100,

		// Texture resources (not to be used in vertex elements)

		// Usage of bit fields are a bit different here
		// 	HIGH              12       8   7   6   5   4   3      LOW 
		// ┌─────────────────┬─────────┬───┬───┬───┬───┬───┬───────┐
		// | 1 0 0 0 0 0 0 0 │ 0 0 0 0 │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 0 0 |
		// ├─────────────────┼─────────┼───┼───┼── ┼───┼───┼───────┤
		// │ Sampler         │ \    /  │ B │ M │ S │ A │ R │ \Size/│      
		// │                 │  Data   │ U │ U │ H │ R │ E │ or 100│                   
		// │                 │  TYPE   │ F │ L │ A │ R │ C │ for   │                    
		// │                 │ 0-DFLT  │ F │ T │ D │ A │ T │ cube  │                      
		// │                 │ 1-INT   │ E │ I │ O │ Y │   │       │
		// │                 │ 2-UINT  │ R │ S │ W │   │   │       │
		// │                 │         │   │ A │   │   │   │       │
		// │                 │         │   │ M │   │   │   │       │
		// │                 │         │   │ P │   │   │   │       │
		// │                 │         │   │ L │   │   │   │       │
		// │                 │         │   │ E │   │   │   │       │
		// └─────────────────┴─────────┴───┴───┴───┴───┴───┴───────┘                            

		// Rect texture means no mipmapping, texel fetch is non-normalized
		Tex1D                       = 0b10000000'0000'00000'001,
		Tex1D_Array                 = 0b10000000'0000'00010'001,
		Tex1D_Shadow                = 0b10000000'0000'00100'001,
		Tex1D_ShadowArray           = 0b10000000'0000'00110'001,
		Tex2D                       = 0b10000000'0000'00000'010,
		Tex2D_Rect                  = 0b10000000'0000'00001'010,
		Tex2D_Rect_Shadow           = 0b10000000'0000'00101'010,
		Tex2D_Array                 = 0b10000000'0000'00010'010,
		Tex2D_Shadow                = 0b10000000'0000'00100'010,
		Tex2D_ShadowArray           = 0b10000000'0000'00110'010,
		Tex2D_Multisample           = 0b10000000'0000'01000'010,
		Tex2D_MultisampleArray      = 0b10000000'0000'01010'010,

		Tex3D                       = 0b10000000'0000'00000'011,

		TexCube                     = 0b10000000'0000'00000'100,
		TexCubeShadow               = 0b10000000'0000'00100'100,

		Tex1D_Int                   = 0b10000000'0001'00000'001,
		Tex1D_Int_Array             = 0b10000000'0001'00010'001,
		Tex2D_Int                   = 0b10000000'0001'00000'010,
		Tex2D_Int_Rect              = 0b10000000'0001'00001'010,
		Tex2D_Int_Array             = 0b10000000'0001'00010'010,
		Tex2D_Int_Multisample       = 0b10000000'0001'01000'010,
		Tex2D_Int_MultisampleArray  = 0b10000000'0001'01010'010,
		Tex3D_Int                   = 0b10000000'0001'00000'011,
		TexCube_Int                 = 0b10000000'0001'00000'100,

		Tex1D_Uint                  = 0b10000000'0010'00000'001,
		Tex2D_Uint_Rect             = 0b10000000'0010'00001'010,
		Tex1D_Uint_Array            = 0b10000000'0010'00010'001,
		Tex2D_Uint                  = 0b10000000'0010'00000'010,
		Tex2D_Uint_Array            = 0b10000000'0010'00010'010,
		Tex2D_Uint_Multisample      = 0b10000000'0010'01000'010,
		Tex2D_Uint_MultisampleArray = 0b10000000'0010'01010'010,
		Tex3D_Uint                  = 0b10000000'0010'00000'011,
		TexCube_Uint                = 0b10000000'0010'00000'100,

		// Buffer textures are for passing unfiltered data, can only be accessed via texelFetch
		BufferTexture               = 0b10000000'0000'10000'000,
		BufferTextureInt            = 0b10000000'0001'10000'000,
		BufferTextureUint           = 0b10000000'0010'10000'000
);
#pragma endregion

// These masks let us extract info from our ShaderDataType structures

const uint32_t ShaderDataType_TypeMask  = 0b11111111'000000'000'000;
const uint32_t ShaderDataType_Size1Mask = 0b00000000'000000'000'111;
const uint32_t ShaderDataType_Size2Mask = 0b00000000'000000'111'000;

/*
* This section let's us map built in and GLM types to their corresponding ShaderDataType
* 
* for intance, GetShaderDataType<float>() returns ShaderDataType::Float
*/
#pragma region Type to ShaderData Mapping
template <typename T>
constexpr ShaderDataType GetShaderDataType() {
	return ShaderDataType::None;
}

template <> constexpr inline ShaderDataType GetShaderDataType<float>() {
	return ShaderDataType::Float;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::vec2>() {
	return ShaderDataType::Float2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::vec3>() {
	return ShaderDataType::Float3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::vec4>() {
	return ShaderDataType::Float4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat2>() {
	return ShaderDataType::Mat2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat2x3>() {
	return ShaderDataType::Mat2x3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat2x4>() {
	return ShaderDataType::Mat2x4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat3>() {
	return ShaderDataType::Mat3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat3x2>() {
	return ShaderDataType::Mat3x2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat3x4>() {
	return ShaderDataType::Mat3x4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat4>() {
	return ShaderDataType::Mat4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat4x2>() {
	return ShaderDataType::Mat4x2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::mat4x3>() {
	return ShaderDataType::Mat4x3;
}

template <> constexpr inline ShaderDataType GetShaderDataType<double>() {
	return ShaderDataType::Double;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dvec2>() {
	return ShaderDataType::Double2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dvec3>() {
	return ShaderDataType::Double3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dvec4>() {
	return ShaderDataType::Double4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat2>() {
	return ShaderDataType::Dmat2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat2x3>() {
	return ShaderDataType::Dmat2x3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat2x4>() {
	return ShaderDataType::Dmat2x4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat3>() {
	return ShaderDataType::Dmat3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat3x2>() {
	return ShaderDataType::Dmat3x2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat3x4>() {
	return ShaderDataType::Dmat3x4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat4>() {
	return ShaderDataType::Dmat4;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat4x2>() {
	return ShaderDataType::Dmat4x2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::dmat4x3>() {
	return ShaderDataType::Dmat4x3;
}

template <> constexpr inline ShaderDataType GetShaderDataType<int>() {
	return ShaderDataType::Int;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::ivec2>() {
	return ShaderDataType::Int2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::ivec3>() {
	return ShaderDataType::Int3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::ivec4>() {
	return ShaderDataType::Int4;
}

template <> constexpr inline ShaderDataType GetShaderDataType<unsigned int>() {
	return ShaderDataType::Uint;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::uvec2>() {
	return ShaderDataType::Uint2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::uvec3>() {
	return ShaderDataType::Uint3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::uvec4>() {
	return ShaderDataType::Uint4;
}

template <> constexpr inline ShaderDataType GetShaderDataType<bool>() {
	return ShaderDataType::Bool;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::bvec2>() {
	return ShaderDataType::Bool2;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::bvec3>() {
	return ShaderDataType::Bool3;
}
template <> constexpr inline ShaderDataType GetShaderDataType<glm::bvec4>() {
	return ShaderDataType::Bool4;
}
#pragma endregion 

/// <summary>
/// Represents the underlying element data type of a ShaderDataType
/// </summary>
ENUM(ShaderDataTypecode, uint32_t,
	 None    = 0,
	 Float   = 0b00000001'000000'000'000,
	 Matrix  = 0b00000010'000000'000'000,
	 Int     = 0b00000100'000000'000'000,
	 Uint    = 0b00001000'000000'000'000,
	 Double  = 0b00010000'000000'000'000,
	 MatrixD = 0b00100000'000000'000'000,
	 Bool    = 0b01000000'000000'000'000,
	 Texture = 0b10000000'000000'000'000
);

/// <summary>
/// Gets the underlying ShaderDataTypecode for the given ShaderDataType
/// This is the underlying element type for the shader data type (ex, float2->float)
/// </summary>
/// <param name="type">The shader data type to examine</param>
/// <returns>The ShaderDataTypecode corresponding to the type</returns>
constexpr ShaderDataTypecode GetShaderDataTypeCode(ShaderDataType type) {
	return (ShaderDataTypecode)((uint32_t)type & ShaderDataType_TypeMask);
}

/// <summary>
/// Gets the size of the underlying ShaderDataType in bytes
/// </summary>
/// <param name="type">The type to determine the size for</param>
/// <returns>The size in bytes, or 0 if the type is invalid</returns>
constexpr uint32_t ShaderDataTypeSize(ShaderDataType type)
{
	if (type == ShaderDataType::Uint64)
		return sizeof(uint64_t);

	ShaderDataTypecode typeCode = GetShaderDataTypeCode(type);
	switch (typeCode) {
		case ShaderDataTypecode::Float:
		case ShaderDataTypecode::Int:
		case ShaderDataTypecode::Uint:
			return 4 * ((uint32_t)type & ShaderDataType_Size1Mask);
		case ShaderDataTypecode::Matrix:
			return 4 * ((uint32_t)type & ShaderDataType_Size1Mask) * (((uint32_t)type & ShaderDataType_Size2Mask) >> 3);
		case ShaderDataTypecode::Double:
			return 8 * ((uint32_t)type & ShaderDataType_Size1Mask);
		case ShaderDataTypecode::MatrixD:
			return 8 * ((uint32_t)type & ShaderDataType_Size1Mask) * (((uint32_t)type & ShaderDataType_Size2Mask) >> 3);
		case ShaderDataTypecode::Bool:
			return (uint32_t)type & ShaderDataType_Size1Mask;
		case ShaderDataTypecode::Texture:
			return 4;
		default:
			LOG_WARN("Cannot determine size for shader typecode: {}", typeCode);
			return 0;
	}
}

/// <summary>
/// Gets the number of underlying components for a given ShaderDataType
/// </summary>
/// <param name="type">The type to determine the number of components for</param>
/// <returns>The number of components in the type</returns>
constexpr uint32_t ShaderDataTypeComponentCount(ShaderDataType type)
{
	ShaderDataTypecode typeCode = (ShaderDataTypecode)((uint32_t)type & ShaderDataType_TypeMask);
	switch (typeCode) {
		case ShaderDataTypecode::Float:
		case ShaderDataTypecode::Int:
		case ShaderDataTypecode::Uint:
		case ShaderDataTypecode::Double:
		case ShaderDataTypecode::Bool:
			return (uint32_t)type & ShaderDataType_Size1Mask;
		case ShaderDataTypecode::Matrix:
		case ShaderDataTypecode::MatrixD:
			return (uint32_t)type & ShaderDataType_Size1Mask * (((uint32_t)type & ShaderDataType_Size2Mask) >> 3);
			return (uint32_t)type & ShaderDataType_Size1Mask;
		default:
			LOG_WARN(false, "Unknown ShaderDataType! {}", type);
			return 1;
	}
}

/// <summary>
/// Handles mapping a GL data type to a ShaderDataType
/// </summary>	
constexpr ShaderDataType FromGLShaderDataType(GLenum glType) {
	switch (glType) {
		case GL_FLOAT:				                        return ShaderDataType::Float;
		case GL_FLOAT_VEC2:			                        return ShaderDataType::Float2;
		case GL_FLOAT_VEC3:			                        return ShaderDataType::Float3;
		case GL_FLOAT_VEC4:			                        return ShaderDataType::Float4;
		case GL_FLOAT_MAT2:			                        return ShaderDataType::Mat2;
		case GL_FLOAT_MAT3:			                        return ShaderDataType::Mat3;
		case GL_FLOAT_MAT4:			                        return ShaderDataType::Mat4;
		case GL_FLOAT_MAT2x3:		                        return ShaderDataType::Mat2x3;
		case GL_FLOAT_MAT2x4:		                        return ShaderDataType::Mat2x4;
		case GL_FLOAT_MAT3x2:		                        return ShaderDataType::Mat3x2;
		case GL_FLOAT_MAT3x4:		                        return ShaderDataType::Mat3x4;
		case GL_FLOAT_MAT4x2:		                        return ShaderDataType::Mat4x2;
		case GL_FLOAT_MAT4x3:		                        return ShaderDataType::Mat4x3;
		case GL_INT:				                        return ShaderDataType::Int;
		case GL_INT_VEC2:			                        return ShaderDataType::Int2;
		case GL_INT_VEC3:			                        return ShaderDataType::Int3;
		case GL_INT_VEC4:			                        return ShaderDataType::Int4;
		case GL_UNSIGNED_INT:		                        return ShaderDataType::Uint;
		case GL_UNSIGNED_INT_VEC2:	                        return ShaderDataType::Uint2;
		case GL_UNSIGNED_INT_VEC3:	                        return ShaderDataType::Uint3;
		case GL_UNSIGNED_INT_VEC4:	                        return ShaderDataType::Uint4;
		case GL_UNSIGNED_INT64_ARB:	                        return ShaderDataType::Uint64;
		case GL_DOUBLE:				                        return ShaderDataType::Double;
		case GL_DOUBLE_VEC2:		                        return ShaderDataType::Double2;
		case GL_DOUBLE_VEC3:		                        return ShaderDataType::Double3;
		case GL_DOUBLE_VEC4:		                        return ShaderDataType::Double4;
		case GL_DOUBLE_MAT2:		                        return ShaderDataType::Dmat2;
		case GL_DOUBLE_MAT3:		                        return ShaderDataType::Dmat3;
		case GL_DOUBLE_MAT4:		                        return ShaderDataType::Dmat4;
		case GL_DOUBLE_MAT2x3:		                        return ShaderDataType::Dmat2x3;
		case GL_DOUBLE_MAT2x4:		                        return ShaderDataType::Dmat2x4;
		case GL_DOUBLE_MAT3x2:		                        return ShaderDataType::Dmat3x2;
		case GL_DOUBLE_MAT3x4:		                        return ShaderDataType::Dmat3x4;
		case GL_DOUBLE_MAT4x2:		                        return ShaderDataType::Dmat4x2;
		case GL_DOUBLE_MAT4x3:		                        return ShaderDataType::Dmat4x3;
		case GL_BOOL:				                        return ShaderDataType::Bool;
		case GL_BOOL_VEC2:			                        return ShaderDataType::Bool2;
		case GL_BOOL_VEC3:			                        return ShaderDataType::Bool3;
		case GL_BOOL_VEC4:			                        return ShaderDataType::Bool4;

			// Textures

		case GL_SAMPLER_1D:							        return ShaderDataType::Tex1D;
		case GL_SAMPLER_1D_ARRAY:					        return ShaderDataType::Tex1D_Array;
		case GL_SAMPLER_1D_SHADOW:					        return ShaderDataType::Tex1D_Shadow;
		case GL_SAMPLER_1D_ARRAY_SHADOW:			        return ShaderDataType::Tex1D_ShadowArray;

		case GL_SAMPLER_2D:							        return ShaderDataType::Tex2D;
		case GL_SAMPLER_2D_RECT:					        return ShaderDataType::Tex2D_Rect;
		case GL_SAMPLER_2D_RECT_SHADOW:				        return ShaderDataType::Tex2D_Rect_Shadow;
		case GL_SAMPLER_2D_ARRAY:					        return ShaderDataType::Tex2D_Array;
		case GL_SAMPLER_2D_SHADOW:					        return ShaderDataType::Tex2D_Shadow;
		case GL_SAMPLER_2D_ARRAY_SHADOW:			        return ShaderDataType::Tex2D_ShadowArray;
		case GL_SAMPLER_2D_MULTISAMPLE:				        return ShaderDataType::Tex2D_Multisample;
		case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:		        return ShaderDataType::Tex2D_MultisampleArray;

		case GL_SAMPLER_3D:							        return ShaderDataType::Tex3D;
		case GL_SAMPLER_CUBE:						        return ShaderDataType::TexCube;
		case GL_SAMPLER_CUBE_SHADOW:				        return ShaderDataType::TexCubeShadow;

			// Buffer Textures

		case GL_SAMPLER_BUFFER:						        return ShaderDataType::BufferTexture;
		case GL_INT_SAMPLER_BUFFER:					        return ShaderDataType::BufferTextureInt;
		case GL_UNSIGNED_INT_SAMPLER_BUFFER:		        return ShaderDataType::BufferTextureUint;

			// Integer textures

		case GL_INT_SAMPLER_1D:						        return ShaderDataType::Tex1D_Int;
		case GL_INT_SAMPLER_1D_ARRAY:				        return ShaderDataType::Tex1D_Int_Array;
		case GL_INT_SAMPLER_2D:						        return ShaderDataType::Tex2D_Int;
		case GL_INT_SAMPLER_2D_RECT:				        return ShaderDataType::Tex2D_Int_Rect;
		case GL_INT_SAMPLER_2D_ARRAY:				        return ShaderDataType::Tex2D_Int_Array;
		case GL_INT_SAMPLER_2D_MULTISAMPLE:			        return ShaderDataType::Tex2D_Int_Multisample;
		case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:	        return ShaderDataType::Tex2D_Int_MultisampleArray;

		case GL_INT_SAMPLER_3D:						        return ShaderDataType::Tex3D_Int;
		case GL_INT_SAMPLER_CUBE:					        return ShaderDataType::TexCube_Int;

			// Unsigned int textures

		case GL_UNSIGNED_INT_SAMPLER_1D:					return ShaderDataType::Tex1D_Uint;
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:				return ShaderDataType::Tex1D_Uint_Array;
		case GL_UNSIGNED_INT_SAMPLER_2D:					return ShaderDataType::Tex2D_Uint;
		case GL_UNSIGNED_INT_SAMPLER_2D_RECT:				return ShaderDataType::Tex2D_Uint_Rect;
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:				return ShaderDataType::Tex2D_Uint_Array;
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:		return ShaderDataType::Tex2D_Uint_Multisample;
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:	return ShaderDataType::Tex2D_Uint_MultisampleArray;

		case GL_UNSIGNED_INT_SAMPLER_3D:					return ShaderDataType::Tex3D_Uint;
		case GL_UNSIGNED_INT_SAMPLER_CUBE:					return ShaderDataType::TexCube_Uint;

		default:                                            return ShaderDataType::None;
	}
}

/// <summary>
/// Handles mapping from a ShaderDataTypecode to an underlying GL data type
/// </summary>
constexpr GLenum ToGLElementType(ShaderDataTypecode typecode) {
	switch (typecode) {
		case ShaderDataTypecode::Float:
		case ShaderDataTypecode::Matrix:
			return GL_FLOAT;
		case ShaderDataTypecode::Int:
			return GL_INT;
		case ShaderDataTypecode::Uint:
			return GL_UNSIGNED_INT;
		case ShaderDataTypecode::Double:
		case ShaderDataTypecode::MatrixD:
			return GL_DOUBLE;
		case ShaderDataTypecode::Bool:
			return GL_BOOL;
		default:
			LOG_ASSERT(false, "Unknown Shader Data Typecode!"); return 0;
	}
}
