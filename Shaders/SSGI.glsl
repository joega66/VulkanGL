#include "CameraCommon.glsl"
#include "LightingCommon.glsl"
#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"
#include "SSGICommon.glsl"
#include "RayTracingCommon.glsl"

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 sceneColorSize = imageSize(SceneColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, sceneColorSize.xy)))
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 screenUV = vec2(screenCoords) / vec2(sceneColorSize);

	SurfaceData surface;
	MaterialData material;

	UnpackGBuffers(screenUV, screenCoords, surface, material);

	const vec3 eyeDir = normalize( Camera.Position - surface.WorldPosition );
	const vec3 scatterDir = reflect( -eyeDir, surface.WorldNormal );
	const vec3 csScatterDir = vec3( normalize( Camera.WorldToView * vec4( scatterDir, 0.0) ) );
	const vec3 csOrigin = vec3( Camera.WorldToView * vec4( surface.WorldPosition, 1 ) );

	vec2 hitPixel;
	vec3 csHitPoint;
	vec3 indirectSpecular = vec3(0);

	if (TraceScreenSpaceRay(
		csOrigin,
		csScatterDir,
		Camera.ViewToClip,
		SceneDepth,
		vec2(sceneColorSize),
		Camera.clipData,
		0.1,	// thickness
		8.0,	// stride
		0.0,	// jitter
		75.0,	// steps
		-Camera.clipData.z, // max distance
		hitPixel,
		csHitPoint)
		)
	{
		indirectSpecular = imageLoad(SceneColor, ivec2(hitPixel)).rgb;

		const vec3 hitNormal = LoadNormal(ivec2(hitPixel));
		indirectSpecular *= dot(hitNormal, scatterDir) > 0.0 ? 0.0 : 1.0;
		indirectSpecular *= dot(hitNormal, surface.WorldNormal) > 0.9 ? 0.0 : 1.0;
	}
	else
	{
		//indirectSpecular = SampleCubemap(_Skybox, _SkyboxSampler, scatterDir ).rgb;
	}

	const vec3 directLighting = imageLoad(SceneColor, screenCoords).rgb;
	const vec3 lo = material.SpecularColor * indirectSpecular + directLighting;

	imageStore(SceneColor, screenCoords, vec4(lo, 1.0));
}