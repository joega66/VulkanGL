layout(location = 0) out vec4 OutColor;
layout(location = 0) in vec3 InPosition;

uniform samplerCube Skybox;

void main()
{
	OutColor = texture(Skybox, InPosition);
}