#pragma once
#include "../GLShader.h"

class FullscreenVS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class FullscreenFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

//std::array<glm::vec3, 4> VertexPositions =
//{
//	glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f)
//};
//std::array<glm::vec2, 4> VertexUVs =
//{
//	glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f)
//};
//std::array<uint32, 6> Indices =
//{
//	0, 1, 2, 1, 3, 2
//};
//GLIndexBufferRef IndexBuffer = GLCreateIndexBuffer(IF_R32_UINT, Indices.size(), EResourceUsage::None, Indices.data());
//GLVertexBufferRef PositionVertexBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, VertexPositions.size(), EResourceUsage::None, VertexPositions.data());
//GLVertexBufferRef TextureCoordinateVertexBuffer = GLCreateVertexBuffer(IF_R32G32_SFLOAT, VertexUVs.size(), EResourceUsage::None, VertexUVs.data());
