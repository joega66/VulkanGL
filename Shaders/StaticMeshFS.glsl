#include "MaterialCommon.glsl"

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec3 InTangent;

const float ParallaxStrength = 0.1f;

#define PARALLAX_RAYMARCHING_INTERPOLATE

#ifndef PARALLAX_RAYMARCHING_SEARCH_STEPS
#define PARALLAX_RAYMARCHING_SEARCH_STEPS 0
#endif

vec2 ParallaxRaymarching(vec2 uv, vec2 viewDir)
{
	const int PARALLAX_RAYMARCHING_STEPS = 10;

	vec2 uvOffset = vec2(0.0f);
	float stepSize = 1.0 / PARALLAX_RAYMARCHING_STEPS;
	vec2 uvDelta = viewDir * (stepSize * ParallaxStrength);

	float stepHeight = 1;
	float surfaceHeight = texture(Bump, uv).r;

	vec2 prevUVOffset = uvOffset;
	float prevStepHeight = stepHeight;
	float prevSurfaceHeight = surfaceHeight;

	for (int i = 1; i < PARALLAX_RAYMARCHING_STEPS && stepHeight > surfaceHeight; i++)
	{
		prevUVOffset = uvOffset;
		prevStepHeight = stepHeight;
		prevSurfaceHeight = surfaceHeight;

		uvOffset -= uvDelta;
		stepHeight -= stepSize;
		surfaceHeight = texture(Bump, uv + uvOffset).r;
	}

#if PARALLAX_RAYMARCHING_SEARCH_STEPS
	for (int i = 0; i < PARALLAX_RAYMARCHING_SEARCH_STEPS; i++)
	{
		uvDelta *= 0.5;
		stepSize *= 0.5;

		if (stepHeight < surfaceHeight)
		{
			uvOffset += uvDelta;
			stepHeight += stepSize;
		}
		else
		{
			uvOffset -= uvDelta;
			stepHeight -= stepSize;
		}
		surfaceHeight = texture(Bump, uv + uvOffset).r;
	}
#elif defined(PARALLAX_RAYMARCHING_INTERPOLATE)
	float prevDifference = prevStepHeight - prevSurfaceHeight;
	float difference = surfaceHeight - stepHeight;
	float t = prevDifference / (prevDifference + difference);
	uvOffset = prevUVOffset - uvDelta * t;
#endif

	return uvOffset;
}

MaterialParams GetMaterial()
{
	// @todo Make me a uniform.
	const mat4 InvLocalToWorld = inverse(LocalToWorld.Transform);
	const vec3 ObjSpaceNormal = normalize(InNormal);

	MaterialParams Material;
	Material.Position = (LocalToWorld.Transform * vec4(InPosition, 1.0f)).xyz;
	Material.Normal = normalize(mat3(transpose(InvLocalToWorld)) * ObjSpaceNormal);

	vec2 MaterialUV = InUV;
	vec3 ObjSpaceTangent = normalize(InTangent);

	if (HasBumpMap)
	{
		const vec3 ObjSpaceViewPos = (InvLocalToWorld * vec4(View.Position, 1.0f)).xyz;
		const vec3 ObjSpaceViewDir = normalize(ObjSpaceViewPos - InPosition);
		const mat3 ObjToTangent = mat3(
			ObjSpaceTangent,
			cross(ObjSpaceNormal, ObjSpaceTangent),
			ObjSpaceNormal
		);

		vec3 TanSpaceViewDir = normalize(ObjToTangent * ObjSpaceViewDir);
		TanSpaceViewDir.xy /= (TanSpaceViewDir.z + 0.42f);

		vec2 uvOffset = ParallaxRaymarching(InUV, TanSpaceViewDir.xy);

		MaterialUV += uvOffset;
	}

	Material.Albedo = texture(Diffuse, MaterialUV).rgb;
	Material.Roughness = 0.25f;
	Material.Shininess = 0.0f;

	if (HasSpecularMap)
	{
		Material.Specular = texture(Specular, MaterialUV).rgb;
	}
	else
	{
		Material.Specular = vec3(1.0f);
	}

	if (HasOpacityMap)
	{
		Material.Alpha = texture(Opacity, MaterialUV).r;
	}
	else
	{
		Material.Alpha = 1.0f;
	}

	return Material;
}