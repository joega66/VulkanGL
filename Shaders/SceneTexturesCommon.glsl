
layout(binding = 0, set = SCENE_TEXTURES_SET) uniform sampler2D SceneDepth;

vec3 ScreenToWorld(vec2 UV)
{
	float Depth = texture(SceneDepth, UV).r;
	vec2 ClipSpace = (UV - 0.5f) * 2.0f;
	vec4 ClipSpaceH = vec4(ClipSpace, Depth, 1.0f);
	mat4 WorldToClipInv = inverse(View.WorldToClip); // @todo Make uniform
	vec4 WorldSpace = WorldToClipInv * ClipSpaceH;
	WorldSpace.xyz /= WorldSpace.w;
	return WorldSpace.xyz;
}