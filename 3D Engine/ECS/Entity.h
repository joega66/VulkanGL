#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr uint32 InvalidID = std::numeric_limits<uint32>::max();

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

	uint32 GetEntityID() const;

	explicit operator bool() const;

private:
	friend class EntityManager; // So that only the entity manager can assign entity ids.

	uint32 EntityID;

	Entity(uint32 EntityID);
};