#include "CameraCommon.glsl"
#define VOXEL_SET 1
#include "VoxelsCommon.glsl"
#include "LightingCommon.glsl"

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1

layout(constant_id = 0) const uint cLightType = 0;

layout(push_constant) uniform DirectionalLightConstants
{
	vec4 L;
	vec4 Radiance;
};

void GetDirectionalLightParams(inout LightParams Light)
{
	Light.L = L.xyz;
	Light.Radiance = Radiance.rgb;
}

void GetPointLightParams(inout LightParams Light, SurfaceData Surface)
{
	const vec3 FragToLight = L.xyz - Surface.WorldPosition;
	const float Distance = length(FragToLight);
	const float Attenuation = 1.0 / (Distance * Distance);

	Light.L = normalize(FragToLight);
	Light.Radiance = Radiance.rgb * Attenuation;
}

float TraceShadowCone(vec3 WorldPosition, vec3 WorldNormal, vec3 LightDir)
{
	const float ConeAngle = 60.0;
	const float Aperture = atan(radians(ConeAngle / 2.0));
	const vec3 StartPosition = WorldPosition + WorldNormal * VOXEL_SIZE;
	const vec3 VolumeUV = TransformWorldToVoxelUVW(StartPosition);

	float Dist = VOXEL_SIZE;
	float Visibility = 0.0;

	while (Visibility < 1.0 && Dist < 1.0)
	{
		const float Diameter = 2.0 * Aperture * Dist;
		const float MipLevel = log2(Diameter / VOXEL_SIZE);
		const float VisibilitySample = textureLod(RadianceVolume, VolumeUV + LightDir * Dist, MipLevel).a;
		Visibility += (1.0 - Visibility) * VisibilitySample; // alpha = alpha + (1 - alpha)alpha2
		Dist += Diameter;
	}

	return Visibility;
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 SceneColorSize = imageSize(SceneColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, SceneColorSize.xy)))
		return;

	const ivec2 ScreenCoords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 ScreenUV = vec2(ScreenCoords) / vec2(SceneColorSize);

	SurfaceData Surface;
	MaterialData Material;

	UnpackGBuffers(ScreenUV, ScreenCoords, Surface, Material);

	const vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	LightParams Light;

	if (cLightType == DIRECTIONAL_LIGHT)
	{
		GetDirectionalLightParams(Light);
	}
	else if (cLightType == POINT_LIGHT)
	{
		GetPointLightParams(Light, Surface);
	}

	vec3 Lo = DirectLighting(V, Light, Surface, Material);

	Lo *= TraceShadowCone(Surface.WorldPosition, Surface.WorldNormal, Light.L);

	const vec3 LoTotal = Lo + imageLoad(SceneColor, ScreenCoords).rgb;

	imageStore(SceneColor, ScreenCoords, vec4(LoTotal, 1.0));
}