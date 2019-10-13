layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec3 InPosition;

layout(set = 1, binding = 0) uniform samplerCube Skybox;

void main()
{
	OutColor = texture(Skybox, InPosition);
}