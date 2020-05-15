#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
inline ComponentType& ComponentArray<ComponentType>::AddComponent(Entity& Entity, ComponentType&& Component)
{
	const std::size_t ArrayIndex = Components.size();

	Components.emplace_back(std::move(Component));

	EntityToArrayIndex[Entity.GetEntityID()] = ArrayIndex;

	ArrayIndexToEntity[ArrayIndex] = Entity.GetEntityID();

	NewEntities.push_back(Entity);

	return Components[ArrayIndex];
}

template<typename ComponentType>
inline ComponentArray<ComponentType>::ComponentArray()
{
}

template<typename ComponentType>
inline ComponentType& ComponentArray<ComponentType>::GetComponent(Entity& Entity)
{
	const std::size_t ArrayIndex = EntityToArrayIndex.at(Entity.GetEntityID());
	return Components[ArrayIndex];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::OnComponentCreated(ComponentEvent<ComponentType> ComponentEvent)
{
	ComponentCreatedEvents.push_back(ComponentEvent);
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::NotifyOnComponentCreatedEvents()
{
	for (auto Entity : NewEntities)
	{
		auto& Component = GetComponent(Entity);
		std::for_each(ComponentCreatedEvents.begin(), ComponentCreatedEvents.end(), [&] (auto& Callback)
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
	const std::size_t Back = Components.size() - 1;
	const std::size_t MovedEntity = ArrayIndexToEntity.at(Back);
	const std::size_t MoveTo = EntityToArrayIndex.at(Entity.GetEntityID());
	
	if (Components.size() > 1)
	{
		Components[MoveTo] = std::move(Components.back());
		EntityToArrayIndex[MovedEntity] = MoveTo;
		ArrayIndexToEntity[MoveTo] = MovedEntity;
	}

	Components.pop_back();

	ArrayIndexToEntity.erase(Back);
	EntityToArrayIndex.erase(Entity.GetEntityID());
}