#include "View.glsl"

layout(location = 0) in vec3 Position;
layout(location = 0) out vec3 OutTexCoord;

void main()
{
	gl_Position = (View.Projection * vec4(mat3(View.View) * Position, 0.0f)).xyzz;
	OutTexCoord = Position;
}