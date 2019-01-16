#pragma once
#include <Platform/Platform.h>
#include <typeindex>

template<typename TComponent>
class ComponentArray;

class Entity;

class ComponentSystem
{
public:
	virtual void Update() {}
	virtual void OnRemove(std::type_index Type, Entity& Entity) {}

	template<typename TComponent>
	void Listen()
	{
		ComponentArray<TComponent>::Get().OnRemoveListener(*this);
	}
};

class IComponentArray;

class ComponentSystemManager
{
	friend class EntityManager;
	template<typename TComponent>
	friend class ComponentArray;
	friend class CoreEngine;

private:
	std::vector<std::reference_wrapper<ComponentSystem>> ComponentSystems;
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;

	void DestroyEntity(Entity& Entity);
	void AddComponentArray(IComponentArray& ComponentArray);
	void AddComponentSystem(ComponentSystem& ComponentSystem);
	void Update();
};

extern class ComponentSystemManager ComponentSystemManager;