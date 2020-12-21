#pragma once
#include "ComponentArray.h"

template<typename ComponentType>
inline ComponentType& ComponentArray<ComponentType>::AddComponent(Entity& entity, ComponentType&& component)
{
	const std::size_t arrayIndex = _Components.size();

	_Components.emplace_back(std::move(component));

	_EntityToArrayIndex[entity.GetEntityID()] = arrayIndex;

	_ArrayIndexToEntity[arrayIndex] = entity.GetEntityID();

	_NewEntities.push_back(entity);

	return _Components[arrayIndex];
}

template<typename ComponentType>
inline ComponentArray<ComponentType>::ComponentArray()
{
}

template<typename ComponentType>
inline ComponentType& ComponentArray<ComponentType>::GetComponent(Entity& entity)
{
	const std::size_t arrayIndex = _EntityToArrayIndex.at(entity.GetEntityID());
	return _Components[arrayIndex];
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::OnComponentCreated(ComponentEvent<ComponentType> componentEvent)
{
	_ComponentCreatedEvents.push_back(componentEvent);
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::NotifyOnComponentCreatedEvents()
{
	for (auto entity : _NewEntities)
	{
		auto& component = GetComponent(entity);
		std::for_each(_ComponentCreatedEvents.begin(), _ComponentCreatedEvents.end(), [&] (auto& callback)
		{
			callback(entity, component);
		});
	}

	_NewEntities.clear();
}

template<typename ComponentType>
inline bool ComponentArray<ComponentType>::HasComponent(Entity& entity) const
{
	return _EntityToArrayIndex.find(entity.GetEntityID()) != _EntityToArrayIndex.end();
}

template<typename ComponentType>
inline void ComponentArray<ComponentType>::RemoveComponent(Entity& entity)
{
	const std::size_t back = _Components.size() - 1;
	const std::size_t movedEntity = _ArrayIndexToEntity.at(back);
	const std::size_t moveTo = _EntityToArrayIndex.at(entity.GetEntityID());
	
	if (_Components.size() > 1)
	{
		_Components[moveTo] = std::move(_Components.back());

		_EntityToArrayIndex[movedEntity] = moveTo;

		_ArrayIndexToEntity[moveTo] = movedEntity;
	}

	_Components.pop_back();

	_ArrayIndexToEntity.erase(back);

	_EntityToArrayIndex.erase(entity.GetEntityID());
}