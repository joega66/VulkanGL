#pragma once
#include "ComponentSystems/ComponentSystem.h"

class Entity;

class IComponentArray
{
public:
	virtual void RemoveComponent(Entity* Entity) = 0;
};

template<typename TComponent>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray()
	{
		GComponentSystemManager.AddComponentArray(this);
	}

	template<typename ...Args>
	TComponent& AddComponent(Entity* Entity, Args&& ...InArgs)
	{
		Components.emplace((std::uintptr_t)Entity, TComponent(std::forward<Args>(InArgs)...));
		return Components[(std::uintptr_t)Entity];
	}

	TComponent& GetComponent(Entity* Entity)
	{
		return Components[(std::uintptr_t)Entity];
	}

	bool HasComponent(Entity* Entity)
	{
		return Contains(Components, (std::uintptr_t)Entity);
	}

	void RemoveComponent(Entity* Entity)
	{
		Components.erase((std::uintptr_t)Entity);
	}

	static ComponentArray<TComponent>& Get()
	{
		static ComponentArray<TComponent> ComponentArray;
		return ComponentArray;
	}

private:
	// @todo Object pool for components
	Map<std::uintptr_t, TComponent> Components;

};