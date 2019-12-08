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

	SpecializationInfo = Material.CreateSpecializationInfo();

	MeshSet = drm::CreateDescriptorSet();
	MeshSet->Write(LocalToWorldUniform, ShaderBinding(0));
	MeshSet->Update();
}