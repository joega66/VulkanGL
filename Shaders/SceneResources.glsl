#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require

#if defined (IMAGE_SET) || defined(IMAGE_3D_SET)
#extension GL_EXT_shader_image_load_formatted : require
#endif

#ifdef TEXTURE_SET
layout(binding = 0, set = TEXTURE_SET) uniform sampler2D _Textures[];
#endif

#ifdef TEXTURE_3D_SET
layout(binding = 0, set = TEXTURE_3D_SET) uniform sampler3D _Textures3D[];
#endif

#ifdef CUBEMAP_SET
layout(binding = 0, set = CUBEMAP_SET) uniform samplerCube _Cubemaps[];
#endif

#ifdef IMAGE_SET
layout(binding = 0, set = IMAGE_SET) uniform image2D _Images[];
#endif

#ifdef IMAGE_3D_SET
layout(binding = 0, set = IMAGE_3D_SET) uniform image3D _Images3D[];
#endif

#if defined(TEXTURE_SET)

vec4 Load(uint textureIdx, ivec2 location)
{
	return texelFetch(_Textures[textureIdx], location, 0);
}

ivec2 TextureSize(uint textureIdx, int level)
{
	return textureSize(_Textures[textureIdx], level);
}

vec4 Sample2D(uint textureIdx, vec2 uv)
{
	return texture(_Textures[textureIdx], uv);
}

#endif

#if defined(TEXTURE_3D_SET)

vec4 TexelFetch(uint textureIdx, ivec3 location, level)
{
	return texelFetch(_Textures3D[textureIdx], location, level);
}

#endif

#if defined(CUBEMAP_SET)

vec4 SampleCubemap(uint textureIdx, vec3 uv)
{
	return texture(_Cubemaps[textureIdx], uv);
}

#endif

#if defined(IMAGE_SET)

vec4 ImageLoad(uint imageIdx, ivec2 location)
{
	return imageLoad(_Images[imageIdx], location);
}

void ImageStore(uint imageIdx, ivec2 location, vec4 value)
{
	imageStore(_Images[imageIdx], location, value);
}

ivec2 ImageSize(uint imageIdx)
{
	return imageSize(_Images[imageIdx]);
}

#endif

#if defined(IMAGE_3D_SET)

void ImageStore(uint imageIdx, ivec3 location, vec4 value)
{
	imageStore(_Images3D[imageIdx], location, value);
}

#endif

#endif