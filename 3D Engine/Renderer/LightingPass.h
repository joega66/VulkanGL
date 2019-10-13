#pragma once
#include <Components/CStaticMesh.h>
#include <DRM.h>

class LightingPassDrawPlan
{
public:
	LightingPassDrawPlan(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform);

	void BindDescriptorSets(RenderCommandList& CmdList, const class SceneProxy& Scene) const;
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::ShaderRef VertShader;
	drm::ShaderRef FragShader;
	drm::DescriptorSetRef DescriptorSet;
	SpecializationInfo SpecInfo;
	const MeshElement& Element;
};