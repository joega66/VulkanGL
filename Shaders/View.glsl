#include "Common.glsl"

uniform ViewUniform
{
	// World to View matrix.
	mat4 View;
	// View to Projective matrix.
	mat4 Projection;
	// View position.
	vec3 Position;
	// Aspect ratio.
	float AspectRatio;
} View;