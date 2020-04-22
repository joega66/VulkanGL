#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require

#ifdef TEXTURES_SET
layout(binding = 0, set = TEXTURES_SET) uniform sampler2D Textures[];
#endif

#endif