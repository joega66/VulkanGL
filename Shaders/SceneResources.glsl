#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require

#ifdef TEXTURES_SET
layout(binding = 0, set = TEXTURES_SET) uniform texture2D Textures[];
#endif

#ifdef SAMPLERS_SET
layout(binding = 0, set = SAMPLERS_SET) uniform sampler Samplers[];
#endif

vec4 Sample2D(uint TextureID, uint SamplerID, vec2 UV)
{
	return texture(sampler2D(Textures[nonuniformEXT(TextureID)], Samplers[nonuniformEXT(SamplerID)]), UV);
}

#endif