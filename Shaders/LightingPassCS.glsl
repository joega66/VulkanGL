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
	const vec3 StartPosition = WorldPosition + WorldNormal * _VoxelSize.x;
	const vec3 VolumeUV = TransformWorldToVoxelUVW(StartPosition);

	float Dist = _VoxelSize.x;
	float Visibility = 0.0;

	while (Visibility < 1.0 && Dist < 1.0)
	{
		const float Diameter = 2.0 * Aperture * Dist;
		const float MipLevel = log2(Diameter / _VoxelSize.x);
		const float VisibilitySample = textureLod(RadianceVolume, VolumeUV + LightDir * Dist, MipLevel).a;
		Visibility += (1.0 - Visibility) * VisibilitySample; // alpha = alpha + (1 - alpha)alpha2
		Dist += Diameter;
	}

	return Visibility;
}

vec3 TraceCone(vec3 StartPosition, vec3 Direction, float ConeAngle)
{
	const float Aperture = atan(radians(ConeAngle / 2.0));
	const vec3 VolumeUV = TransformWorldToVoxelUVW(StartPosition);

	float Dist = _VoxelSize.x;
	vec3 Li = vec3(0.0);
	float Alpha = 0.0;

	while (Alpha < 1.0 && Dist < 1.0)
	{
		const float Diameter = 2.0 * Aperture * Dist;
		const float MipLevel = log2(Diameter / _VoxelSize.x);
		const vec4 LiSample = textureLod(RadianceVolume, VolumeUV + Direction * Dist, MipLevel).rgba;
		Li = Alpha * Li + (1.0 - Alpha) * LiSample.a * LiSample.rgb; // c = a * c + (1 - a)a2 * c
		Alpha += (1.0 - Alpha) * LiSample.a; // alpha = alpha + (1 - alpha)alpha2
		Dist += Diameter;
	}
	
	return Li / ( Dist * float( VOXEL_GRID_SIZE ) );
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

	vec3 V = normalize(Camera.Position - Surface.WorldPosition);

	LightParams Light;

	if (cLightType == DIRECTIONAL_LIGHT)
	{
		GetDirectionalLightParams(Light);
	}
	else if (cLightType == POINT_LIGHT)
	{
		GetPointLightParams(Light, Surface);
	}

	vec3 Lo = vec3(0.0);

	Lo += DirectLighting(V, Light, Surface, Material);

	Lo *= TraceShadowCone(Surface.WorldPosition, Surface.WorldNormal, Light.L);

	vec3 Up = abs(Surface.WorldNormal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 Tangent = normalize(cross(Up, Surface.WorldNormal));
	vec3 Bitangent = cross(Surface.WorldNormal, Tangent);

	vec3 RayStart = Surface.WorldPosition + 3.0 * Surface.WorldNormal;

	vec3 IndirectDiffuse = vec3(0.0);
	
	float DiffuseCone = 60.0;
	float Theta = 45.0;
	float SinTheta = sin(Theta);
	float CosTheta = cos(Theta);
	float Phi[] = { 0.0, 90.0, 180.0, 270.0 };

	for (int i = 0; i < 4; i++)
	{
		vec3 Cartesian;
		Cartesian.x = cos(Phi[i]) * SinTheta;
		Cartesian.y = sin(Phi[i]) * SinTheta;
		Cartesian.z = CosTheta;

		vec3 SampleDir = normalize(Tangent * Cartesian.x + Bitangent * Cartesian.y + Surface.WorldNormal * Cartesian.z);

		float NdotL = max(dot(Surface.WorldNormal, SampleDir), 0.0);

		IndirectDiffuse += Material.DiffuseColor * TraceCone(RayStart, SampleDir, DiffuseCone) * NdotL;
	}

	IndirectDiffuse += Material.DiffuseColor * TraceCone(RayStart, Surface.WorldNormal, DiffuseCone);

	Lo += IndirectDiffuse;

	/** Indirect specular. */

	/*vec3 RayStart = Surface.WorldPosition + Surface.WorldNormal * VOXEL_SIZE;
	vec3 Eye = normalize( Surface.WorldPosition - Camera.Position );
	vec3 RayDir = normalize( reflect(Eye, Surface.WorldNormal) );
	
	vec3 IndirectSpecular = TraceCone( RayStart, RayDir, 35.0 );

	BRDFContext BRDFContext;
	BRDF_InitContext(BRDFContext, Surface.WorldNormal, V, RayDir);

	Lo += Specular_BRDF(BRDFContext, Material.SpecularColor, Material.Roughness) * IndirectSpecular * BRDFContext.NdotL;*/

	vec3 LoTotal = Lo + imageLoad(SceneColor, ScreenCoords).rgb;

	imageStore(SceneColor, ScreenCoords, vec4(LoTotal, 1.0));
}