#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
inline ComponentType & ComponentArray<ComponentType>::AddComponent(Entity& Entity, ComponentType&& Component)
{
	uint32 ArrayIndex = -1;

	if (!FreeList.empty())
	{
		ArrayIndex = FreeList.front();
		FreeList.pop_front();
		Components[ArrayIndex] = std::move(Component);
	}
	else
	{
		Components.emplace_back(std::move(Component));
		ArrayIndex = Components.size() - 1;
	}

	EntityToArrayIndex[Entity.GetEntityID()] = ArrayIndex;

	// Later notify component events that this component was created.
	NewEntities.push_back(Entity);

	// Return the newly created component.
	return Components[ArrayIndex];
}

template<typename ComponentType>
inline ComponentArray<ComponentType>::ComponentArray()
{
}

template<typename ComponentType>
inline ComponentType & ComponentArray<ComponentType>::GetComponent(Entity& Entity)
{
	const uint32 ArrayIndex = EntityToArrayIndex.at(Entity.GetEntityID());
	return Components[ArrayIndex];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::NewComponentCallback(ComponentCallback<ComponentType> ComponentCallback)
{
	ComponentCreatedCallbacks.push_back(ComponentCallback);
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::NotifyComponentCreatedEvents()
{
	for (auto Entity : NewEntities)
	{
		auto& Component = GetComponent(Entity);
		std::for_each(ComponentCreatedCallbacks.begin(), ComponentCreatedCallbacks.end(), [&] (auto& Callback)
		{
			Callback(Entity, Component);
		});
	}

	NewEntities.clear();
}

template<typename ComponentType>
inline bool ComponentArray<ComponentType>::HasComponent(Entity& Entity) const
{
	return EntityToArrayIndex.find(Entity.GetEntityID()) != EntityToArrayIndex.end();
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::RemoveComponent(Entity& Entity)
{
	const uint32 ArrayIndex = EntityToArrayIndex.at(Entity.GetEntityID());
	FreeList.push_back(ArrayIndex);
	EntityToArrayIndex.erase(Entity.GetEntityID());
}