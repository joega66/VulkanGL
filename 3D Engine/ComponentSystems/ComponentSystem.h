#pragma once
#include <Platform/Platform.h>
#include <typeindex>

template<typename TComponent>
class ComponentArray;

class ComponentSystem
{
public:
	virtual void Update() {}
	virtual void OnRemove(std::type_index Type, class Entity& Entity) {}

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
	void AddComponentArray(IComponentArray& ComponentArray);

private:
	friend class EntityManager;

	std::vector<std::reference_wrapper<ComponentSystem>> ComponentSystems;
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;

	void DestroyEntity(uint64 EntityID);
};

extern ComponentSystemManager GComponentSystemManager;