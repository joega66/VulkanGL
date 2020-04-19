#pragma once
#include <ECS/Component.h>
#include <Physics/Physics.h>

class Bounds : public Component
{
public:
	/** World-space bounding box. */
	BoundingBox Box;
};