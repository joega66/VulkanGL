#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material* Material,
	drm::DescriptorSet&& SurfaceSet,
	const std::vector<Submesh>& Submeshes,
	drm::Buffer&& LocalToWorldUniform)
	: Material(Material)
	, SurfaceSet(std::move(SurfaceSet))
	, Submeshes(&Submeshes)
	, LocalToWorldUniform(std::move(LocalToWorldUniform))
{
}