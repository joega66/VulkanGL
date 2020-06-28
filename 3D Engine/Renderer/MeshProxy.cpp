#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material* Material,
	gpu::DescriptorSet&& SurfaceSet,
	const std::vector<Submesh>& Submeshes,
	gpu::Buffer&& LocalToWorldUniform)
	: Material(Material)
	, SurfaceSet(std::move(SurfaceSet))
	, Submeshes(&Submeshes)
	, LocalToWorldUniform(std::move(LocalToWorldUniform))
{
}