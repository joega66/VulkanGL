#pragma once
#include "System.h"

class Entity;

class IComponentArray
{
public:
	virtual void RemoveComponent(Entity& Entity) {};
	virtual std::shared_ptr<void> CopyComponent(Entity& Entity) { return nullptr; };
	virtual void AddComponent(Entity& Entity, std::shared_ptr<void> Data) {};
	virtual bool HasComponent(Entity& Entity) { return false; };
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
	void OnRemoveListener(ISystem& ISystem);
	static ComponentArray<TComponent>& Get();

private:
	// @todo Object pool for components
	HashTable<uint64, TComponent> Components;
	std::vector<std::reference_wrapper<ISystem>> OnRemoveListeners;
};