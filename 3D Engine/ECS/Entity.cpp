#include "Entity.h"
#include "EntityManager.h"

Entity::Entity()
	: EntityID(InvalidID)
{
}

Entity::Entity(uint64 EntityID)
	: EntityID(EntityID)
{
}

uint64 Entity::GetEntityID() const
{
	return EntityID;
}

Entity::operator bool() const
{
	return EntityID != InvalidID;
}