#include "Common.glsl"

layout(location = 0) in vec3 Position;
#ifndef DEPTH_ONLY
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
#endif

uniform LocalToWorldUniform
{
	mat4 Transform;
} LocalToWorld;

#ifndef DEPTH_ONLY
layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
#endif

void SetVSInterpolants(vec4 WorldPosition)
{
#ifndef DEPTH_ONLY
	OutPosition = WorldPosition.xyz;
	OutUV = UV;
	OutNormal = mat3(transpose(inverse(LocalToWorld.Transform))) * Normal;
#endif
}

vec4 GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(Position, 1.0f);
}