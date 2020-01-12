#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material& Material,
	const std::vector<Submesh>& Submeshes,
	const drm::BufferRef& LocalToWorldUniform)
	: Material(Material)
	, LocalToWorldUniform(LocalToWorldUniform)
	, Submeshes(Submeshes)
{
	MaterialSet = Material.CreateDescriptorSet();

	SpecializationInfo = Material.CreateSpecializationInfo();

	// @todo Make virtual
	MeshSet = drm::CreateDescriptorSet();
	MeshSet->Write(LocalToWorldUniform, 0);
	MeshSet->Update();
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