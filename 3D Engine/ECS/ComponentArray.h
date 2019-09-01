#pragma once
#include "System.h"

class Entity;

class IComponentArray
{
public:
	virtual void RemoveComponent(Entity& Entity) = 0;
	virtual std::shared_ptr<void> CopyComponent(Entity& Entity) = 0;
	virtual void AddComponent(Entity& Entity, std::shared_ptr<void> Data) = 0;
	virtual bool HasComponent(Entity& Entity) = 0;
};

template<typename TComponent>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray();
	TComponent& GetComponent(Entity& Entity);
	template<typename ...Args>
	TComponent& AddComponent(Entity& Entity, Args&& ...InArgs);
	virtual void AddComponent(Entity& Entity, std::shared_ptr<void> Data) final;
	virtual std::shared_ptr<void> CopyComponent(Entity& Entity) final;
	virtual bool HasComponent(Entity& Entity) final;
	virtual void RemoveComponent(Entity& Entity) final;
	static ComponentArray<TComponent>& Get();

private:
	// @todo Object pool for components
	HashTable<uint64, TComponent> Components;
};