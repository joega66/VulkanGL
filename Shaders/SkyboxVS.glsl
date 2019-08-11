#include "SceneCommon.glsl"
layout(location = 0) in vec3 Position;
layout(location = 0) out vec3 OutTexCoord;

void main()
{
	vec4 OutPosition = View.ViewToClip * vec4(mat3(View.WorldToView) * Position, 0.0f);
	gl_Position = OutPosition.xyzz;
	OutTexCoord = Position;
	OutTexCoord.y *= -1;
}