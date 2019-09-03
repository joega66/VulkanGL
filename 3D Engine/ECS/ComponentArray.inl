#pragma once
#include "ComponentArray.h"

template<typename TComponent>
template<typename ...Args>
inline TComponent & ComponentArray<TComponent>::AddComponent(Entity & Entity, Args && ...InArgs)
{
	Components.emplace(Entity.GetEntityID(), std::move(TComponent(std::forward<Args>(InArgs)...)));
	return Components[Entity.GetEntityID()];
}

template<typename TComponent>
inline void ComponentArray<TComponent>::AddComponent(Entity & Entity, std::shared_ptr<void> Data)
{
	Components.emplace(Entity.GetEntityID(), *std::static_pointer_cast<TComponent>(Data));
}

template<typename TComponent>
inline TComponent & ComponentArray<TComponent>::GetComponent(Entity & Entity)
{
	return Components[Entity.GetEntityID()];
}

template<typename TComponent>
inline std::shared_ptr<void> ComponentArray<TComponent>::CopyComponent(Entity & Entity)
{
	return std::make_shared<TComponent>(GetComponent(Entity));
}

template<typename TComponent>
inline bool ComponentArray<TComponent>::HasComponent(Entity & Entity)
{
	return Contains(Components, Entity.GetEntityID());
}

template<typename TComponent>
inline void ComponentArray<TComponent>::RemoveComponent(Entity & Entity)
{
	Components.erase(Entity.GetEntityID());
}