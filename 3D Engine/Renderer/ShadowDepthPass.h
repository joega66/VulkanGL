#pragma once
#include <DRM.h>

class ShadowDepthPass
{
public:
	struct PassDescriptors
	{
		drm::DescriptorSetRef ShadowProxy;
	};

	ShadowDepthPass(const class MeshProxy& MeshProxy);
	void BindDescriptorSets(drm::CommandList& CmdList, const class MeshProxy& MeshProxy, const PassDescriptors& Pass) const;
	void SetPipelineState(PipelineStateInitializer& PSOInit, const class MeshProxy& MeshProxy) const;
	void Draw(drm::CommandList& CmdList, const struct MeshElement& MeshElement) const;

private:
	drm::ShaderRef VertShader;
	drm::ShaderRef FragShader;
};