#pragma once
#include "System.h"

class Entity;

template<typename ComponentType>
using ComponentCallback = std::function<void(Entity& Entity, ComponentType& Component)>;

/** ComponentArray interface. Only meant to be implemented by ComponentArray. */
class IComponentArray
{
public:
	virtual bool HasComponent(Entity& Entity) const = 0;
	virtual void RemoveComponent(Entity& Entity) = 0;
	virtual void NotifyComponentCreatedEvents() = 0;
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
	void NewComponentCallback(ComponentCallback<ComponentType> ComponentCallback);

	/** Notify callbacks that a component was created. */
	virtual void NotifyComponentCreatedEvents() final;

	/** Check if entity has a component. */
	virtual bool HasComponent(Entity& Entity) const final;

	/** Remove component from an entity. */
	virtual void RemoveComponent(Entity& Entity) final;

	inline const std::unordered_map<uint32, std::size_t>& GetEntities() const { return EntityToArrayIndex; }

private:
	/** Component pool. */
	std::vector<ComponentType> Components;

	/** List of free indices in the component array. */
	std::list<std::size_t> FreeList;

	/** Maps an entity to its array index. */
	std::unordered_map<uint32, std::size_t> EntityToArrayIndex;

	/** Component created events. */
	std::vector<ComponentCallback<ComponentType>> ComponentCreatedCallbacks;

	/** Entities that were given a component this frame. */
	std::vector<Entity> NewEntities;
};