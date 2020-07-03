#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants
{
	uint _Skybox;
	uint _Sampler;
};

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = SampleCubemap(_Skybox, _Sampler, inPosition);
}