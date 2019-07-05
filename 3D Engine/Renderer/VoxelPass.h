#pragma once

#include "MaterialShader.h"
#include <DRM.h>

//class VoxelPassDrawingPlan
//{
//public:
//	VoxelPassDrawingPlan(const struct MeshElement& Element, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform);
//
//	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
//	GraphicsPipelineState GetGraphicsPipeline() const;
//	void SetUniforms(RenderCommandList& CmdList, const View& View);
//	void Draw(RenderCommandList& CmdList) const;
//
//private:
//	Ref<LightingPassBaseVS> VertShader;
//	Ref<LightingPassBaseFS> FragShader;
//
//	std::vector<MaterialSource> Materials;
//	std::vector<UniformSource> Uniforms;
//	const struct MeshElement& Element;
//};