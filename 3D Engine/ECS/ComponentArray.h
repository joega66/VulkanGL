#pragma once
#include "System.h"
#include <functional>

class Entity;

template<typename ComponentType>
using ComponentEvent = std::function<void(Entity& Entity, ComponentType& Component)>;

/** ComponentArray interface. Only meant to be implemented by ComponentArray. */
class IComponentArray
{
public:
	virtual bool HasComponent(Entity& Entity) const = 0;
	virtual void RemoveComponent(Entity& Entity) = 0;
	virtual void NotifyOnComponentCreatedEvents() = 0;
};

template<typename ComponentType>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray();

	/** Get an entity's component. */
	ComponentType& GetComponent(Entity& Entity);

	/** Add a component to an entity. */
	ComponentType& AddComponent(Entity& Entity, ComponentType&& Component);

	/** Add a callback for when a component is created.	*/
	void OnComponentCreated(ComponentEvent<ComponentType> ComponentEvent);

	/** Notify callbacks that a component was created. */
	virtual void NotifyOnComponentCreatedEvents() final;

	/** Check if entity has a component. */
	virtual bool HasComponent(Entity& Entity) const final;

	/** Remove component from an entity. */
	virtual void RemoveComponent(Entity& Entity) final;

	inline const std::unordered_map<std::size_t, std::size_t>& GetEntities() const { return EntityToArrayIndex; }

private:
	/** Component pool. */
	std::vector<ComponentType> Components;

	/** Maps an entity to its array index. */
	std::unordered_map<std::size_t, std::size_t> EntityToArrayIndex;

	/** Maps an array index to an entity. */
	std::unordered_map<std::size_t, std::size_t> ArrayIndexToEntity;

	/** Component created events. */
	std::vector<ComponentEvent<ComponentType>> ComponentCreatedEvents;

	/** Entities that were given a component this frame. */
	std::vector<Entity> NewEntities;
};