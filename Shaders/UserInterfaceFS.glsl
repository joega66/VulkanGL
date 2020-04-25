#version 450
#define TEXTURE_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	uint TextureID;
	uint SamplerID;
};

layout(location = 0) in vec2 InUV;
layout(location = 1) in vec4 InColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = InColor * Sample2D(TextureID, SamplerID, InUV);
}