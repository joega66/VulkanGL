#include "CameraCommon.glsl"

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 outTexCoord;

void main()
{
	vec4 outPosition = _Camera.viewToClip * vec4(mat3(_Camera.worldToView) * position, 0.0f);
	gl_Position = outPosition.xyzz;
	outTexCoord = position;
}