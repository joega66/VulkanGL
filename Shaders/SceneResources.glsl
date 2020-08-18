#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require

#if defined(TEXTURE_SET) || defined(TEXTURE_3D_SET) || defined(CUBEMAP_SET)
#extension GL_EXT_samplerless_texture_functions : require
#endif

#if defined(IMAGE_3D_SET)
#extension GL_EXT_shader_image_load_formatted : require
#endif

#ifdef TEXTURE_SET
layout(binding = 0, set = TEXTURE_SET) uniform texture2D _Textures[];
#endif

#ifdef TEXTURE_3D_SET
layout(binding = 0, set = TEXTURE_3D_SET) uniform texture3D _Textures3D[];
#endif

#ifdef SAMPLER_SET
layout(binding = 0, set = SAMPLER_SET) uniform sampler _Samplers[];
#endif

#ifdef CUBEMAP_SET
layout(binding = 0, set = CUBEMAP_SET) uniform textureCube _Cubemaps[];
#endif

#ifdef IMAGE_3D_SET
layout(binding = 0, set = IMAGE_3D_SET) uniform image3D _Images3D[];
#endif

#if defined(TEXTURE_SET)

vec4 Load(uint textureID, ivec2 location)
{
	return texelFetch(_Textures[textureID], location, 0);
}

ivec2 TextureSize(uint textureID, int level)
{
	return textureSize(_Textures[textureID], level);
}

#if defined(SAMPLER_SET)

vec4 Sample2D(uint textureID, uint samplerID, vec2 uv)
{
	return texture(sampler2D(_Textures[textureID], _Samplers[samplerID]), uv);
}

#endif

#endif

#if defined(TEXTURE_3D_SET)

vec4 TexelFetch(uint textureID, ivec3 location, level)
{
	return texelFetch(_Textures3D[textureID], location, level);
}

#endif

#if defined(CUBEMAP_SET)

vec4 SampleCubemap(uint textureID, uint samplerID, vec3 uv)
{
	return texture(samplerCube(_Cubemaps[textureID], _Samplers[samplerID]), uv);
}

#endif

#if defined(IMAGE_3D_SET)

void ImageStore(uint imageID, ivec3 location, vec4 value)
{
	imageStore(_Images3D[imageID], location, value);
}

#endif

#endif