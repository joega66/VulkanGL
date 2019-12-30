#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr uint64 InvalidID = std::numeric_limits<uint64>::max();

	Entity();

	bool operator==(const Entity& Entity) const
	{
		return EntityID == Entity.GetEntityID();
	}

	bool operator!=(const Entity& Entity) const
	{
		return EntityID != Entity.GetEntityID();
	}

	bool operator<(const Entity& Entity) const
	{
		return EntityID < Entity.GetEntityID();
	}

	uint64 GetEntityID() const;

	explicit operator bool() const;

private:
	friend class EntityManager; // So that only the entity manager can assign entity ids.

	uint64 EntityID;

	Entity(uint64 EntityID);
};