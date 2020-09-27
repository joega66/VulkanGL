#define CAMERA_SET 0
#include "CameraCommon.glsl"
#include "LightingCommon.glsl"
#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"
#include "SSGICommon.glsl"
#include "RayTracingCommon.glsl"

vec3 ComputeSSR(ivec2 screenCoords, SurfaceData surface, ONB onb, MaterialData material, float alpha)
{
	float cosH;
	const vec3 eyeDir = normalize(_Camera.position - surface.worldPosition);
	const vec3 halfwayDir = ONB_Transform(onb, GGX_ImportanceSample(alpha, cosH));
	const vec3 scatterDir = normalize(2 * dot(eyeDir, halfwayDir) * halfwayDir - eyeDir);
	const vec3 csScatterDir = vec3(normalize(_Camera.worldToView * vec4(scatterDir, 0.0)));
	const vec3 csOrigin = vec3(_Camera.worldToView * vec4(surface.worldPosition, 1));

	vec2 hitPixel;
	vec3 csHitPoint;
	vec3 li = vec3(0);

	const float thicc = 0.1;
	const float stride = 16.0;
	const float jitter = RandomFloat();
	const float steps = 25.0;

	if (TraceScreenSpaceRay(
		csOrigin,
		csScatterDir,
		_Camera.viewToClip,
		_SceneDepth,
		_Camera.screenDims,
		_Camera.clipData,
		thicc, stride, jitter, steps,
		-_Camera.clipData.z,
		hitPixel, csHitPoint))
	{
		li = imageLoad(_DirectLighting, ivec2(hitPixel)).rgb;

		const vec3 hitNormal = LoadNormal(ivec2(hitPixel));
		li *= dot(hitNormal, scatterDir) > 0.0 ? 0.0 : 1.0;
		li *= dot(hitNormal, surface.worldNormal) > 0.9 ? 0.0 : 1.0;
	}
	else
	{
		//li = SampleCubemap(_Skybox, _SkyboxSampler, scatterDir ).rgb;
	}

	const float ndoth = max(dot(surface.worldNormal, halfwayDir), 0);
	const float ndotv = max(dot(surface.worldNormal, eyeDir), 0);
	const float ndotl = max(dot(surface.worldNormal, scatterDir), 0);
	const float vdoth = max(dot(eyeDir, halfwayDir), 1e-6);

	const float ndf = NormalGGX(ndoth, alpha);
	const float g = SmithGF(ndotv, ndotl, alpha);
	const vec3 fresnel = FresnelSchlick(material.specularColor, ndotl);
	const vec3 specular = (ndf * g * fresnel) / max(4.0 * ndotv * ndotl, 1e-6);

	const float scatteringPdf = GGX_ScatteringPDF(ndf, vdoth, cosH);

	const vec3 threshold = vec3(1);

	vec3 indirectSpecular = min(li * specular * ndotl / scatteringPdf, threshold);

	const vec4 prevSSRColor = imageLoad(_SSRHistory, screenCoords);

	const float blend = (_FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevSSRColor.a)));

	indirectSpecular = mix(prevSSRColor.rgb, indirectSpecular, blend);

	imageStore(_SSRHistory, screenCoords, vec4(indirectSpecular, blend));

	return indirectSpecular;
}

vec3 ComputeSSGI(ivec2 screenCoords, SurfaceData surface, ONB onb, MaterialData material, float alpha)
{
	const vec3 scatterDir = ONB_Transform(onb, RandomCosineDirection());
	const vec3 csScatterDir = vec3(normalize(_Camera.worldToView * vec4(scatterDir, 0.0)));
	const vec3 csOrigin = vec3(_Camera.worldToView * vec4(surface.worldPosition, 1));
	
	vec2 hitPixel;
	vec3 csHitPoint;
	vec3 li = vec3(0);

	const float thicc = 0.1;
	const float stride = 16.0;
	const float jitter = RandomFloat();
	const float steps = 10.0;

	if (TraceScreenSpaceRay(
		csOrigin,
		csScatterDir,
		_Camera.viewToClip,
		_SceneDepth,
		_Camera.screenDims,
		_Camera.clipData,
		thicc, stride, jitter, steps,
		-_Camera.clipData.z,
		hitPixel, csHitPoint))
	{
		li = imageLoad(_DirectLighting, ivec2(hitPixel)).rgb;
	}
	else
	{
		//li = SampleCubemap(_Skybox, _SkyboxSampler, scatterDir).rgb;
	}

	const float scatteringPdf = Lambertian_ScatteringPDF(surface.worldNormal, scatterDir);

	vec3 indirectDiffuse = li * material.baseColor * scatteringPdf;

	const vec4 prevSSGIColor = imageLoad(_SSGIHistory, screenCoords);

	const float blend = (_FrameNumber == 0) ? 1.0f : (1.0f / (1.0f + (1.0f / prevSSGIColor.a)));

	indirectDiffuse = mix(prevSSGIColor.rgb, indirectDiffuse, blend);

	imageStore(_SSGIHistory, screenCoords, vec4(indirectDiffuse, blend));

	return indirectDiffuse;
}

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

	const vec3 indirectDiffuse = ComputeSSGI(screenCoords, surface, onb, material, alpha);

	const vec3 indirectSpecular = ComputeSSR(screenCoords, surface, onb, material, alpha);

	const vec3 directLighting = imageLoad(_DirectLighting, screenCoords).rgb;

	const vec3 lo = indirectDiffuse + indirectSpecular + directLighting;

	imageStore(_SceneColor, screenCoords, vec4(lo, 1.0));
}