#define TEXTURE_SET 0
#include "SceneResources.glsl"

layout(push_constant) uniform PushConstants
{
	layout(offset = 16) uint _Texture;
};

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = inColor * Sample2D(_Texture, inUV);
}