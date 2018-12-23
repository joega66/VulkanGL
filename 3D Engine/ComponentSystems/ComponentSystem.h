#pragma once
#include "Platform/Platform.h"

class ComponentSystem
{
public:
	virtual void RenderUpdate(class Scene& Scene) {}
	virtual void Update(class Scene& Scene) {}
};

class IComponentArray;

class ComponentSystemManager
{
public:
	ComponentSystemManager() = default;

	void UpdateSystems(class Scene& Scene);
	void AddComponentSystem(ComponentSystem& ComponentSystem);
	void AddRenderSystem(ComponentSystem& RenderSystem);
	void AddComponentArray(IComponentArray& ComponentArray);
	void DestroyEntity(const uint64 EntityID);

private:
	friend class EntityManager;

	std::vector<std::reference_wrapper<ComponentSystem>> ComponentSystems;
	std::vector<std::reference_wrapper<ComponentSystem>> RenderSystems;
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;
};

extern ComponentSystemManager GComponentSystemManager;