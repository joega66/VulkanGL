#pragma once
#include "ComponentArray.h"

class Entity
{
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
	friend class EntityManager; // So that only the entity manager can assign entity ids.
	uint64 EntityID;
	Entity(uint64 EntityID);
};