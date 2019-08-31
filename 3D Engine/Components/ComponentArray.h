#pragma once
#include <ECS/System.h>

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

class Entity
{
	friend class EntityManager;
public:
	static constexpr uint64 InvalidID = std::numeric_limits<uint64>::max();

	Entity();

	bool operator==(const Entity& Entity)
	{
		return EntityID == Entity.GetEntityID();
	}

	bool operator!=(const Entity& Entity)
	{
		return EntityID != Entity.GetEntityID();
	}

	template<typename TComponent, typename ...Args>
	TComponent& AddComponent(Args&& ...InArgs)
	{
		return ComponentArray<TComponent>::Get().AddComponent(*this, std::forward<Args>(InArgs)...);
	}

	template<typename TComponent>
	TComponent& GetComponent()
	{
		return ComponentArray<TComponent>::Get().GetComponent(*this);
	}

	template<typename TComponent>
	bool HasComponent()
	{
		return ComponentArray<TComponent>::Get().HasComponent(*this);
	}

	template<typename TComponent>
	void RemoveComponent()
	{
		ComponentArray<TComponent>::Get().RemoveComponent(*this);
	}

	void DestroyEntity();
	uint64 GetEntityID() const;
	explicit operator bool() const;

private:
	uint64 EntityID;

	Entity(uint64 EntityID);
};

#include "ComponentArray.inl"