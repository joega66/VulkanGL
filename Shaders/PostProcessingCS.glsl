#define IMAGE_SET 0
#include "SceneResources.glsl"

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

layout(push_constant) uniform Params { PostProcessingParams _Params; };

layout(local_size_x = 8, local_size_y = 8) in;
void main()
{
	const ivec2 imageSize = ImageSize(_Params._DisplayColor);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, imageSize.xy)))
		return;

	const ivec2 screenCoords = ivec2(gl_GlobalInvocationID.xy);

	vec3 color = ImageLoad(_Params._HDRColor, screenCoords).rgb;

	color *= _Params._ExposureAdjustment;

	color = Uncharted2ToneMapping(color * _Params._ExposureBias);

	const float w = 11.2;

	const vec3 whiteScale = 1.0 / Uncharted2ToneMapping(vec3(w));

	color *= whiteScale;

	color = ApplyGammaCorrection(color);

	ImageStore(_Params._DisplayColor, screenCoords, vec4(color, 1.0));
}