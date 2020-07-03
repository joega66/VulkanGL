#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material* material,
	gpu::DescriptorSet&& surfaceSet,
	const std::vector<Submesh>& submeshes,
	gpu::Buffer&& localToWorldUniform)
	: _Material(material)
	, _SurfaceSet(std::move(surfaceSet))
	, _Submeshes(&submeshes)
	, _LocalToWorldUniform(std::move(localToWorldUniform))
{
}