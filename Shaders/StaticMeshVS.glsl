#include "Common.glsl"

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(binding = 1) uniform LocalToWorldUniform
{
	mat4 Transform;
} LocalToWorld;

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;

void SetVSInterpolants()
{
	OutPosition = vec3(LocalToWorld.Transform * vec4(Position, 1.0f));
	OutUV = UV;
	OutNormal = mat3(transpose(inverse(LocalToWorld.Transform))) * Normal;
}

vec4 GetWorldPosition()
{
	return vec4(OutPosition, 1.0f);
}