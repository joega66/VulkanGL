#include "MaterialCommon.glsl"

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
layout(location = 3) out vec3 OutTangent;

void SetVSInterpolants(vec4 WorldPosition)
{
	OutPosition = Position;
	OutUV = UV;
	OutNormal = Normal;
	OutTangent = Tangent;
}

vec4 GetWorldPosition()
{
	return LocalToWorld.Transform * vec4(Position, 1.0f);
}