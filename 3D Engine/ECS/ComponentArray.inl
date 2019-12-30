#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
template<typename ...Args>
inline ComponentType & ComponentArray<ComponentType>::AddComponent(Entity& Entity, Args &&...InArgs)
{
	Components.emplace(Entity.GetEntityID(), std::move(ComponentType(std::forward<Args>(InArgs)...)));
	return Components[Entity.GetEntityID()];
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
inline void ComponentArray<ComponentType>::NotifyComponentCreated(Entity& Entity, ComponentType& Component)
{
	std::for_each(ComponentCreatedCallbacks.begin(), ComponentCreatedCallbacks.end(), [&](auto& Callback)
	{
		Callback(Entity, Component);
	});
}

template<typename ComponentType>
inline bool ComponentArray<ComponentType>::HasComponent(Entity& Entity) const
{
	return Contains(Components, Entity.GetEntityID());
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::RemoveComponent(Entity& Entity)
{
	Components.erase(Entity.GetEntityID());
}