#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const CMaterial& Material,
	const std::vector<MeshElement>& Elements,
	const drm::BufferRef& LocalToWorldUniform)
	: Material(Material)
	, LocalToWorldUniform(LocalToWorldUniform)
	, Elements(Elements)
{
	MaterialSet = Material.CreateDescriptorSet();
	MaterialSet->Write(LocalToWorldUniform, ShaderBinding(0)); // Temp: LocalToWorld goes in the material set
	MaterialSet->Update();

	SpecInfo = Material.CreateSpecializationInfo();
}