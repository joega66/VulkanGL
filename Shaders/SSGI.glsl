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

	seed = RandomInit(screenCoords, _FrameNumber);

	ONB onb;
	ONB_BuildFromW(onb, surface.worldNormal);

	const float alpha = material.roughness * material.roughness;

	float cosH;
	const vec3 eyeDir = normalize( _Camera.position - surface.worldPosition );
	const vec3 halfwayDir = ONB_Transform( onb, GGX_ImportanceSample( alpha, cosH ) );
	const vec3 scatterDir = normalize( 2 * dot( eyeDir, halfwayDir ) * halfwayDir - eyeDir );
	const vec3 csScatterDir = vec3( normalize( _Camera.worldToView * vec4(scatterDir, 0.0) ) );
	const vec3 csOrigin = vec3( _Camera.worldToView * vec4(surface.worldPosition, 1) );
	
	vec2 hitPixel;
	vec3 csHitPoint;
	vec3 li = vec3(0);

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
		li = imageLoad(_SceneColor, ivec2(hitPixel)).rgb;

		const vec3 hitNormal = LoadNormal(ivec2(hitPixel));
		li *= dot( hitNormal, scatterDir ) > 0.0 ? 0.0 : 1.0;
		li *= dot( hitNormal, surface.worldNormal ) > 0.9 ? 0.0 : 1.0;
	}
	else
	{
		//li = SampleCubemap(_Skybox, _SkyboxSampler, scatterDir ).rgb;
	}

	const float ndoth = max(dot(surface.worldNormal, halfwayDir), 0);
	const float ndotv = max(dot(surface.worldNormal, eyeDir), 0);
	const float ndotl = max(dot(surface.worldNormal, scatterDir), 0);
	const float vdoth = max(dot(eyeDir, halfwayDir), 1e-6);

	const float ndf		= NormalGGX(ndoth, alpha);
	const float g		= SmithGF(ndotv, ndotl, alpha);
	const vec3 fresnel	= FresnelSchlick(material.specularColor, ndotl);
	const vec3 specular = (ndf * g * fresnel) / max(4.0 * ndotv * ndotl, 1e-6);

	const float scatteringPDF = GGX_ScatteringPDF(ndf, vdoth, cosH);

	const vec3 threshold = vec3(1);

	vec3 indirectSpecular = min(li * specular * ndotl / scatteringPDF, threshold);

	const vec4 prevSSGIColor = imageLoad(_SSGIHistory, screenCoords);
	const float blend = (_FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevSSGIColor.a)));
	indirectSpecular = mix(prevSSGIColor.rgb, indirectSpecular, blend);
	imageStore(_SSGIHistory, screenCoords, vec4(indirectSpecular, blend));

	const vec3 directLighting = imageLoad(_SceneColor, screenCoords).rgb;
	const vec3 lo = indirectSpecular + directLighting;
	imageStore(_SceneColor, screenCoords, vec4(lo, 1.0));
}