#include "View.glsl"

layout(location = 0) in vec3 Position;

void main()
{
	gl_Position = View.Projection * View.View * vec4(Position, 1.0f);
}