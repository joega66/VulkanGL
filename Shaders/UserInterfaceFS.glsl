#version 450
#define TEXTURES_SET 1
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	uint TextureID;
};

layout(location = 0) in vec2 InUV;
layout(location = 1) in vec4 InColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = InColor * texture(Textures[nonuniformEXT(TextureID)], InUV);
}