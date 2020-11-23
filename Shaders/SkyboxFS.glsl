#define CUBEMAP_SET 1
#define SAMPLER_SET 2
#include "SceneResources.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Params { SkyboxParams _Params; };

void main()
{
	outColor = SampleCubemap(_Params._Skybox, _Params._Sampler, inPosition);
}