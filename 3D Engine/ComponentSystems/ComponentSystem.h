#pragma once
#include <Platform/Platform.h>
#include <typeindex>

template<typename TComponent>
class ComponentArray;

class ComponentSystem
{
public:
	virtual void RenderUpdate() {}
	virtual void Update() {}
	virtual void OnRemove(std::type_index Type, const class Entity& Entity) {}

	template<typename TComponent>
	void Listen()
	{
		ComponentArray<TComponent>::Get().OnRemoveListener(*this);
	}
};

class IComponentArray;

class ComponentSystemManager
{
public:
	ComponentSystemManager() = default;

	void UpdateSystems();
	void AddComponentSystem(ComponentSystem& ComponentSystem);
	void AddRenderSystem(ComponentSystem& RenderSystem);
	void AddComponentArray(IComponentArray& ComponentArray);

private:
	friend class EntityManager;

	std::vector<std::reference_wrapper<ComponentSystem>> ComponentSystems;
	std::vector<std::reference_wrapper<ComponentSystem>> RenderSystems;
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;

	void DestroyEntity(const uint64 EntityID);
};

extern ComponentSystemManager GComponentSystemManager;