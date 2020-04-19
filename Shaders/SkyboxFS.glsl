layout(location = 0) in vec3 InPosition;

layout(binding = 0, set = 1) uniform samplerCube Skybox;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = texture(Skybox, InPosition);
}