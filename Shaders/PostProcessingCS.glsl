#define TEXTURE_SET 1
#include "SceneResources.glsl"

layout(binding = 0, set = 0, rgba8) uniform writeonly image2D _DisplayColor;
layout(binding = 1, set = 0, rgba16f) uniform readonly image2D _HDRColor;

vec3 Uncharted2ToneMapping(vec3 x)
{
	const float a = 0.15;
	const float b = 0.50;
	const float c = 0.10;
	const float d = 0.20;
	const float e = 0.02;
	const float f = 0.30;

	return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

vec3 ApplyGammaCorrection(vec3 color)
{
	color = pow(color, vec3(1.0 / 2.2));
	return color;
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 imageSize = imageSize(_DisplayColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, imageSize.xy)))
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);

	vec3 color = imageLoad(_HDRColor, screenCoords).rgb;

	color *= _ExposureAdjustment;

	color = Uncharted2ToneMapping(color * _ExposureBias);

	const float w = 11.2;

	const vec3 whiteScale = 1.0 / Uncharted2ToneMapping(vec3(w));

	color *= whiteScale;

	color = ApplyGammaCorrection(color);

	imageStore(_DisplayColor, screenCoords, vec4(color, 1.0));
}