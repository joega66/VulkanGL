#ifndef SCENE_RESOURCES_H
#define SCENE_RESOURCES_H

#extension GL_EXT_nonuniform_qualifier : require

#ifdef SCENE_TEXTURES_SET
layout(binding = 0, set = SCENE_TEXTURES_SET) uniform sampler2D SceneTextures[];
#endif

#endif