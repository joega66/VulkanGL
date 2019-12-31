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

	// @todo Make virtual
	MeshSet = drm::CreateDescriptorSet();
	MeshSet->Write(LocalToWorldUniform, 0);
	MeshSet->Update();
}

void MeshProxy::DrawElements(drm::CommandList& CmdList) const
{
	std::for_each(Elements.begin(), Elements.end(),
		[&] (const MeshElement& MeshElement)
	{
		CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
		CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
	});
}