#define CAMERA_SET 0
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
	const ivec2 sceneColorSize = imageSize(_SceneColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, sceneColorSize.xy)))
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 screenUV = vec2(screenCoords) / vec2(sceneColorSize);

	SurfaceData surface;
	MaterialData material;

	UnpackGBuffers(screenUV, screenCoords, surface, material);

	uint seed = uint(uint(screenCoords.x) * uint(1973) + uint(screenCoords.y) * uint(9277) + _FrameNumber * uint(26699)) | uint(1);
	
	const vec3 eyeDir = normalize( _Camera.position - surface.worldPosition );
	const vec3 scatterDir = reflect( -eyeDir, surface.worldNormal + material.roughness * RandomInUnitSphere( seed ) );
	const vec3 csScatterDir = vec3( normalize( _Camera.worldToView * vec4(scatterDir, 0.0) ) );
	const vec3 csOrigin = vec3( _Camera.worldToView * vec4(surface.worldPosition, 1) );
	
	vec2 hitPixel;
	vec3 csHitPoint;
	vec3 indirectSpecular = vec3(0);

	const float THICC = 0.1;
	const float STRIDE = 16.0;
	const float JITTER = 0.0;
	const float STEPS = 25.0;

	if (TraceScreenSpaceRay(
		csOrigin,
		csScatterDir,
		_Camera.viewToClip,
		_SceneDepth,
		vec2(sceneColorSize),
		_Camera.clipData,
		THICC,
		STRIDE,
		JITTER,
		STEPS,
		-_Camera.clipData.z, // max distance
		hitPixel,
		csHitPoint)
		)
	{
		indirectSpecular = imageLoad(_SceneColor, ivec2(hitPixel)).rgb;

		const vec3 hitNormal = LoadNormal(ivec2(hitPixel));
		indirectSpecular *= dot( hitNormal, scatterDir ) > 0.0 ? 0.0 : 1.0;
		indirectSpecular *= dot( hitNormal, surface.worldNormal ) > 0.9 ? 0.0 : 1.0;

		//float confidence = smoothstep(0, THICC, distance(csOrigin, csHitPoint));
	}
	else
	{
		//indirectSpecular = SampleCubemap(_Skybox, _SkyboxSampler, scatterDir ).rgb;
	}

	indirectSpecular *= material.baseColor;

	const vec4 prevSSGIColor = imageLoad(_SSGIHistory, screenCoords);
	const float blend = (_FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevSSGIColor.a)));
	indirectSpecular = mix(prevSSGIColor.rgb, indirectSpecular, blend);
	imageStore(_SSGIHistory, screenCoords, vec4(indirectSpecular, blend));

	const vec3 directLighting = imageLoad(_SceneColor, screenCoords).rgb;
	const vec3 lo = indirectSpecular + directLighting;
	imageStore(_SceneColor, screenCoords, vec4(lo, 1.0));
}