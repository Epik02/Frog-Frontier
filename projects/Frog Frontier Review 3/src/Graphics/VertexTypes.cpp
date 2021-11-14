#include "VertexTypes.h"
#pragma warning( push )

VertexPosCol* VPC = nullptr;
VertexPosNormCol* VPNC = nullptr;
VertexPosNormTex* VPNT = nullptr;
VertexPosNormTexCol* VPNTC = nullptr;

const std::vector<BufferAttribute> VertexPosCol::V_DECL = {
	BufferAttribute(0, 3, AttributeType::Float, sizeof(VertexPosCol), (size_t)&VPC->Position, AttribUsage::Position),
	BufferAttribute(1, 4, AttributeType::Float, sizeof(VertexPosCol), (size_t)&VPC->Color, AttribUsage::Color),
};
const std::vector<BufferAttribute> VertexPosNormCol::V_DECL = {
	BufferAttribute(0, 3, AttributeType::Float, sizeof(VertexPosNormCol), (size_t)&VPNC->Position, AttribUsage::Position),
	BufferAttribute(1, 4, AttributeType::Float, sizeof(VertexPosNormCol), (size_t)&VPNC->Color, AttribUsage::Color),
	BufferAttribute(2, 3, AttributeType::Float, sizeof(VertexPosNormCol), (size_t)&VPNC->Normal, AttribUsage::Normal),
};
const std::vector<BufferAttribute> VertexPosNormTex::V_DECL = {
	BufferAttribute(0, 3, AttributeType::Float, sizeof(VertexPosNormTex), (size_t)&VPNT->Position, AttribUsage::Position),
	BufferAttribute(2, 3, AttributeType::Float, sizeof(VertexPosNormTex), (size_t)&VPNT->Normal, AttribUsage::Normal),
	BufferAttribute(3, 2, AttributeType::Float, sizeof(VertexPosNormTex), (size_t)&VPNT->UV, AttribUsage::Texture),
};
const std::vector<BufferAttribute> VertexPosNormTexCol::V_DECL = {
	BufferAttribute(0, 3, AttributeType::Float, sizeof(VertexPosNormTexCol), (size_t)&VPNTC->Position, AttribUsage::Position),
	BufferAttribute(1, 4, AttributeType::Float, sizeof(VertexPosNormTexCol), (size_t)&VPNTC->Color, AttribUsage::Color),
	BufferAttribute(2, 3, AttributeType::Float, sizeof(VertexPosNormTexCol), (size_t)&VPNTC->Normal, AttribUsage::Normal),
	BufferAttribute(3, 2, AttributeType::Float, sizeof(VertexPosNormTexCol), (size_t)&VPNTC->UV, AttribUsage::Texture),
};
#pragma warning(pop)