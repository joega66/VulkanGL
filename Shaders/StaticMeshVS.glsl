#include "Common.glsl"

layout(location = 0) in vec3 Position;
#ifndef DEPTH_ONLY
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
#endif

layout(binding = 1) uniform LocalToWorldUniform
{
	mat4 Transform;
} LocalToWorld;

layout(location = 0) out vec3 OutPosition;
#ifndef DEPTH_ONLY
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
#endif

void SetVSInterpolants()
{
	OutPosition = vec3(LocalToWorld.Transform * vec4(Position, 1.0f));
#ifndef DEPTH_ONLY
	OutUV = UV;
	OutNormal = mat3(transpose(inverse(LocalToWorld.Transform))) * Normal;
#endif
}

vec4 GetWorldPosition()
{
	return vec4(OutPosition, 1.0f);
}