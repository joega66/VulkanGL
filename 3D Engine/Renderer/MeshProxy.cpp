#include "MeshProxy.h"

MeshProxy::MeshProxy(
	DRM& Device,
	drm::DescriptorSetRef MeshSet,
	const class Material& Material,
	const std::vector<Submesh>& Submeshes,
	const drm::BufferRef& LocalToWorldUniform)
	: MeshSet(MeshSet)
	, Material(Material)
	, LocalToWorldUniform(LocalToWorldUniform)
	, Submeshes(Submeshes)
{
	MaterialSet = Material.CreateDescriptorSet(Device);
	SpecializationInfo = Material.CreateSpecializationInfo();
}

void MeshProxy::DrawElements(drm::CommandList& CmdList) const
{
	std::for_each(Submeshes.begin(), Submeshes.end(),
		[&] (const Submesh& Submesh)
	{
		CmdList.BindVertexBuffers(Submesh.GetVertexBuffers().size(), Submesh.GetVertexBuffers().data());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0);
	});
}