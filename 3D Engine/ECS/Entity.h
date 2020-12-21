#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr std::size_t _InvalidID = std::numeric_limits<std::size_t>::max();

	Entity();

	bool operator==(const Entity& entity) const
	{
		return _EntityID == entity.GetEntityID();
	}

	bool operator!=(const Entity& entity) const
	{
		return _EntityID != entity.GetEntityID();
	}

	bool operator<(const Entity& entity) const
	{
		return _EntityID < entity.GetEntityID();
	}

	inline std::size_t GetEntityID() const
	{
		return _EntityID;
	};

private:
	friend class EntityManager; // So that only the entity manager can assign entity ids.

	std::size_t _EntityID;

	Entity(std::size_t entityID);
};