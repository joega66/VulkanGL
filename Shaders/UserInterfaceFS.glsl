#version 450
#define TEXTURE_SET 0
#define SAMPLER_SET 1
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	layout(offset = 16) ivec2 _TextureAndSampler;
};

layout(location = 0) in vec2 InUV;
layout(location = 1) in vec4 InColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = InColor * Sample2D(_TextureAndSampler.x, _TextureAndSampler.y, InUV);
}