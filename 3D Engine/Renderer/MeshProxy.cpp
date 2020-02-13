#include "MeshProxy.h"

MeshProxy::MeshProxy(
	DRMDevice& Device,
	drm::DescriptorSetRef SurfaceSet,
	const class Material& Material,
	drm::DescriptorSetRef MaterialSet,
	const std::vector<Submesh>& Submeshes,
	const drm::BufferRef& LocalToWorldUniform)
	: SurfaceSet(SurfaceSet)
	, Material(&Material)
	, MaterialSet(MaterialSet)
	, LocalToWorldUniform(LocalToWorldUniform)
	, Submeshes(Submeshes)
{
}

void MeshProxy::DrawElements(drm::CommandList& CmdList) const
{
	std::for_each(Submeshes.begin(), Submeshes.end(),
		[&] (const Submesh& Submesh)
	{
		CmdList.BindVertexBuffers(Submesh.GetVertexBuffers().size(), Submesh.GetVertexBuffers().data());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0, Submesh.GetIndexType());
	});
}