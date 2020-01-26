#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
template<typename ...Args>
inline ComponentType & ComponentArray<ComponentType>::AddComponent(Entity& Entity, Args &&...InArgs)
{
	// Set the component to alive.
	ComponentStatus[Entity.GetEntityID()] = true;

	// Construct the component.
	Components[Entity.GetEntityID()] = std::move(ComponentType(std::forward<Args>(InArgs)...));

	// Later notify component events that this component was created.
	NewEntities.push_back(Entity);

	// Return the newly created component.
	return Components[Entity.GetEntityID()];
}

template<typename ComponentType>
inline ComponentArray<ComponentType>::ComponentArray()
{
	ComponentStatus.resize(ArraySize, false);
	Components.resize(ArraySize);
}

template<typename ComponentType>
inline ComponentType & ComponentArray<ComponentType>::GetComponent(Entity& Entity)
{
	return Components[Entity.GetEntityID()];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::CopyComponent(Entity& Destination, Entity& Source)
{
	AddComponent(Destination, GetComponent(Source));
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
	return ComponentStatus[Entity.GetEntityID()];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::RemoveComponent(Entity& Entity)
{
	ComponentStatus[Entity.GetEntityID()] = false;
}