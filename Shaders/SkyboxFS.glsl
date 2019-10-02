#include "MaterialCommon.glsl"

layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec3 InPosition;

layout(set = MATERIAL_SET, binding = DIFFUSE_BINDING) uniform samplerCube Skybox;

void main()
{
	OutColor = texture(Skybox, InPosition);
}