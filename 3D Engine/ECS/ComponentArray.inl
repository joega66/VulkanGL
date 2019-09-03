#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
template<typename ...Args>
inline ComponentType & ComponentArray<ComponentType>::AddComponent(Entity & Entity, Args && ...InArgs)
{
	Components.emplace(Entity.GetEntityID(), std::move(ComponentType(std::forward<Args>(InArgs)...)));
	return Components[Entity.GetEntityID()];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::AddComponent(Entity & Entity, std::shared_ptr<void> Data)
{
	Components.emplace(Entity.GetEntityID(), *std::static_pointer_cast<ComponentType>(Data));
}

template<typename ComponentType>
inline ComponentType & ComponentArray<ComponentType>::GetComponent(Entity & Entity)
{
	return Components[Entity.GetEntityID()];
}

template<typename ComponentType>
inline std::shared_ptr<void> ComponentArray<ComponentType>::CopyComponent(Entity & Entity)
{
	return std::make_shared<ComponentType>(GetComponent(Entity));
}

template<typename ComponentType>
inline bool ComponentArray<ComponentType>::HasComponent(Entity & Entity) const
{
	return Contains(Components, Entity.GetEntityID());
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::RemoveComponent(Entity & Entity)
{
	Components.erase(Entity.GetEntityID());
}