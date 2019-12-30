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
	virtual void CopyComponent(Entity& Destination, Entity& Source) = 0;
	virtual void RemoveComponent(Entity& Entity) = 0;
};

template<typename ComponentType>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray() = default;

	/** Get an entity's component. */
	ComponentType& GetComponent(Entity& Entity);

	/** Add a component to an entity. */
	template<typename ...Args>
	ComponentType& AddComponent(Entity& Entity, Args&& ...InArgs);

	/** Add a callback for when a component is created.	*/
	void NewComponentCallback(ComponentCallback<ComponentType> ComponentCallback);

	/** Notify callbacks that the component was created. */
	void NotifyComponentCreated(Entity& Entity, ComponentType& Component);

	/** Check if entity has a component. */
	virtual bool HasComponent(Entity& Entity) const final;
	
	/** Copy component from source entity to destination. */
	virtual void CopyComponent(Entity& Destination, Entity& Source) final;

	/** Remove component from an entity. */
	virtual void RemoveComponent(Entity& Entity) final;

private:
	// @todo Object pool for components
	HashTable<uint64, ComponentType> Components;
	std::vector<ComponentCallback<ComponentType>> ComponentCreatedCallbacks;
};