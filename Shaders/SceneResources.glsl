#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require

#ifdef TEXTURE_SET
layout(binding = 0, set = TEXTURE_SET) uniform texture2D Textures[];
#endif

#ifdef TEXTURE_3D_SET
layout(binding = 0, set = TEXTURE_3D_SET) uniform texture3D Textures3D[];
#endif

#ifdef SAMPLER_SET
layout(binding = 0, set = SAMPLER_SET) uniform sampler Samplers[];
#endif

#ifdef CUBEMAP_SET
layout(binding = 0, set = CUBEMAP_SET) uniform textureCube Cubemaps[];
#endif

#if defined(TEXTURE_SET)

vec4 Load(uint TextureID, ivec2 Location)
{
	return texelFetch(Textures[nonuniformEXT(TextureID)], Location, 0);
}

ivec2 TextureSize(uint TextureID, int Level)
{
	return textureSize(Textures[nonuniformEXT(TextureID)], Level);
}

#if defined(SAMPLER_SET)

vec4 Sample2D(uint TextureID, uint SamplerID, vec2 UV)
{
	return texture(sampler2D(Textures[nonuniformEXT(TextureID)], Samplers[nonuniformEXT(SamplerID)]), UV);
}

#endif

#endif

#if defined(TEXTURE_3D_SET)

vec4 TexelFetch(uint TextureID, ivec3 Location, int Level)
{
	return texelFetch(Textures3D[nonuniformEXT(TextureID)], Location, Level);
}

#endif

#ifdef CUBEMAP_SET
vec4 SampleCubemap(uint TextureID, uint SamplerID, vec3 UV)
{
	return texture(samplerCube(Cubemaps[nonuniformEXT(TextureID)], Samplers[nonuniformEXT(SamplerID)]), UV);
}
#endif

#endif