#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr std::size_t InvalidID = std::numeric_limits<std::size_t>::max();

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

	inline std::size_t GetEntityID() const
	{
		return EntityID;
	};

private:
	friend class EntityManager; // So that only the entity manager can assign entity ids.

	std::size_t EntityID;

	Entity(std::size_t EntityID);
};