#include "Common.glsl"

layout(location = 0) out vec2 OutUV;

void main()
{
	OutUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(OutUV * 2.0f - 1.0f, 0.0f, 1.0f);
}