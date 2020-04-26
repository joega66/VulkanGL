#define TEXTURE_SET 1
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	float ExposureAdjustment;
	float ExposureBias;
};

layout(binding = 0, set = 0, rgba8) uniform writeonly image2D DisplayColor;
layout(binding = 1, set = 0, rgba16f) uniform readonly image2D HDRColor;

vec3 Uncharted2ToneMapping(vec3 X)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;

	return ((X * (A * X + C * B) + D * E) / (X * (A * X + B) + D * F)) - E / F;
}

vec3 ApplyGammaCorrection(vec3 Color)
{
	Color = pow(Color, vec3(1.0 / 2.2));
	return Color;
}

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 ImageSize = imageSize(DisplayColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, ImageSize.xy)))
		return;

	const ivec2 ScreenCoords = ivec2(gl_GlobalInvocationID.xy);

	vec3 Color = imageLoad(HDRColor, ScreenCoords).rgb;

	Color *= ExposureAdjustment;

	Color = Uncharted2ToneMapping(Color * ExposureBias);

	const float W = 11.2;

	const vec3 WhiteScale = 1.0 / Uncharted2ToneMapping(vec3(W));

	Color *= WhiteScale;

	Color = ApplyGammaCorrection(Color);

	imageStore(DisplayColor, ScreenCoords, vec4(Color, 1.0));
}