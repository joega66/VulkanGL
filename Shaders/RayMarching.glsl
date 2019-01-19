#include "View.glsl"
#include "LightingCommon.glsl"
#include "RayMarchingCommon.glsl"

layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec2 InUV;

vec4 Shade(vec3 ViewPosition, vec3 FragPosition, vec3 FragNormal)
{
	vec3 Lo = vec3(0.0);
	vec3 R0 = vec3(0.04);
	vec3 V = normalize(ViewPosition - FragPosition);
	vec3 Albedo = vec3(0.0, 0.0, 1.0);

	R0 = mix(R0, Albedo, Metallic);

	Lo += PointLighting(FragPosition, FragNormal, V, R0, Albedo);
	
	// Gamma
	Lo = Lo / (Lo + vec3(1.0));
	Lo = pow(Lo, vec3(1.0 / 2.2));

	return vec4(Lo, 1.0);
}

void main()
{
	vec3 RayOrigin = View.Position;
	vec3 RayDir = RayDirection(View.AspectRatio, View.FieldOfView, InUV);
	RayDir = normalize(inverse(View.View) * vec4(RayDir, 0.0)).xyz;
	float Distance = RayMarch(RayOrigin, RayDir, MIN_DIST, MAX_DIST);

	if (Distance > MAX_DIST - EPSILON)
	{
		discard;
	}
	else
	{
		vec3 Position = RayOrigin + RayDir * Distance;
		vec3 Normal = CalcNormalSDF(Position);

		OutColor = Shade(View.Position, Position, Normal);

		vec4 ClipSpace = View.Projection * View.View * vec4(Position, 1.0f);
		gl_FragDepth = ClipSpace.z / ClipSpace.w;
	}
}