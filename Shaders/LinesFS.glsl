layout(location = 0) out vec4 OutColor;

uniform ColorUniform
{
	vec4 Color;
} Color;

void main()
{
	OutColor = Color.Color;
}