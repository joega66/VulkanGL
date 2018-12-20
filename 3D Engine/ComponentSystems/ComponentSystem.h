#pragma once
#include "Platform/Platform.h"

class ComponentSystem
{
public:
	virtual void RenderUpdate(class Scene* Scene) {}
	virtual void Update(class Scene* Scene) {}
};

class IComponentArray;

class ComponentSystemManager
{
public:
	ComponentSystemManager() = default;

	void UpdateSystems(class Scene* Scene);
	void AddComponentSystem(ComponentSystem* ComponentSystem);
	void AddRenderSystem(ComponentSystem* RenderSystem);
	void AddComponentArray(IComponentArray* ComponentArray);
	void DestroyEntity(class Entity* Entity);

private:
	friend class EntityManager;

	// @todo std::reference_wrapper
	std::vector<ComponentSystem*> ComponentSystems;
	std::vector<ComponentSystem*> RenderSystems;
	std::vector<IComponentArray*> ComponentArrays;
};

extern ComponentSystemManager GComponentSystemManager;