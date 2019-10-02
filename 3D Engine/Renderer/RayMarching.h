#pragma once
#include <DRMShader.h>

//class RayMarchingFS : public drm::Shader
//{
//public:
//	RayMarchingFS(const ShaderResourceTable& Resources)
//		: drm::Shader(Resources), SceneBindings(Resources)
//	{
//	}
//
//	static const ShaderInfo& GetShaderInfo()
//	{
//		static ShaderInfo Base = { "../Shaders/RayMarching.glsl", "main", EShaderStage::Fragment };
//		return Base;
//	}
//
//	SceneBindings SceneBindings;
//};