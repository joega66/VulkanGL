#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

layout(location = 0) in vec3 InPosition;

layout(push_constant) uniform PushConstants
{
	uint Skybox;
	uint Sampler;
};

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = SampleCubemap(Skybox, Sampler, InPosition);
}