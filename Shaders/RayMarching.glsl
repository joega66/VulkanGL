#include "View.glsl"
#include "MaterialCommon.glsl"
#include "LightingCommon.glsl"
#include "RayMarchingCommon.glsl"

layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec2 InUV;

void main()
{
	vec3 RayOrigin = View.Position;
	vec3 RayDir = RayDirection(View.AspectRatio, View.FieldOfView, InUV);
	RayDir = normalize(inverse(View.WorldToView) * vec4(RayDir, 0.0)).xyz;
	float Distance = RayMarch(RayOrigin, RayDir, MIN_DIST, MAX_DIST);

	if (Distance > MAX_DIST - EPSILON)
	{
		discard;
	}
	else
	{
		vec3 Position = RayOrigin + RayDir * Distance;
		vec3 Normal = CalcNormalSDF(Position);

		MaterialParams Material;
		Material.Position = Position;
		Material.Normal = Normal;
		Material.Albedo = vec3(0.0f, 1.0f, 0.0f);
		Material.Metallic = 0.5f;

		OutColor = Shade(View.Position, Material);

		vec4 ClipSpace = View.WorldToClip * vec4(Position, 1.0f);
		gl_FragDepth = ClipSpace.z / ClipSpace.w;
	}
}