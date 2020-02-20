#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material* Material,
	drm::DescriptorSetRef SurfaceSet,
	const std::vector<Submesh>& Submeshes,
	drm::Buffer&& LocalToWorldUniform)
	: Material(Material)
	, SurfaceSet(SurfaceSet)
	, Submeshes(&Submeshes)
	, LocalToWorldUniform(std::move(LocalToWorldUniform))
{
}

void MeshProxy::DrawElements(drm::CommandList& CmdList) const
{
	std::for_each(Submeshes->begin(), Submeshes->end(),
		[&] (const Submesh& Submesh)
	{
		CmdList.BindVertexBuffers(Submesh.GetVertexBuffers().size(), Submesh.GetVertexBuffers().data());
		CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0, Submesh.GetIndexType());
	});
}