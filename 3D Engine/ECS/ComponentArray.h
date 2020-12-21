#pragma once
#include "System.h"
#include <functional>

class Entity;

template<typename ComponentType>
using ComponentEvent = std::function<void(Entity&, ComponentType&)>;

/** ComponentArray interface. Only meant to be implemented by ComponentArray. */
class IComponentArray
{
public:
	virtual bool HasComponent(Entity& entity) const = 0;
	virtual void RemoveComponent(Entity& entity) = 0;
	virtual void NotifyOnComponentCreatedEvents() = 0;
};

template<typename ComponentType>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray();

	/** Get an entity's component. */
	ComponentType& GetComponent(Entity& entity);

	/** Add a component to an entity. */
	ComponentType& AddComponent(Entity& entity, ComponentType&& component);

	/** Add a callback for when a component is created.	*/
	void OnComponentCreated(ComponentEvent<ComponentType> componentEvent);

	/** Notify callbacks that a component was created. */
	virtual void NotifyOnComponentCreatedEvents() final;

	/** Check if entity has a component. */
	virtual bool HasComponent(Entity& entity) const final;

	/** Remove component from an entity. */
	virtual void RemoveComponent(Entity& entity) final;

	inline const std::unordered_map<std::size_t, std::size_t>& GetEntities() const { return _EntityToArrayIndex; }

private:
	/** Component pool. */
	std::vector<ComponentType> _Components;

	/** Maps an entity to its array index. */
	std::unordered_map<std::size_t, std::size_t> _EntityToArrayIndex;

	/** Maps an array index to an entity. */
	std::unordered_map<std::size_t, std::size_t> _ArrayIndexToEntity;

	/** Component created events. */
	std::vector<ComponentEvent<ComponentType>> _ComponentCreatedEvents;

	/** Entities that were given a component this frame. */
	std::vector<Entity> _NewEntities;
};