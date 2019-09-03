#pragma once
#include "System.h"

class Entity;

class IComponentArray
{
public:
	virtual void RemoveComponent(Entity& Entity) = 0;
	virtual std::shared_ptr<void> CopyComponent(Entity& Entity) = 0;
	virtual void AddComponent(Entity& Entity, std::shared_ptr<void> Data) = 0;
	virtual bool HasComponent(Entity& Entity) const = 0;
};

template<typename ComponentType>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray() = default;
	ComponentType& GetComponent(Entity& Entity);
	template<typename ...Args>
	ComponentType& AddComponent(Entity& Entity, Args&& ...InArgs);
	virtual void AddComponent(Entity& Entity, std::shared_ptr<void> Data) final;
	virtual std::shared_ptr<void> CopyComponent(Entity& Entity) final;
	virtual bool HasComponent(Entity& Entity) const final;
	virtual void RemoveComponent(Entity& Entity) final;

private:
	// @todo Object pool for components
	HashTable<uint64, ComponentType> Components;
};