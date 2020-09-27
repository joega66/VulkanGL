#include "MeshProxy.h"

MeshProxy::MeshProxy(
	const class Material* material,
	const std::vector<Submesh>& submeshes)
	: _Material(material)
	, _Submeshes(&submeshes)
{
}