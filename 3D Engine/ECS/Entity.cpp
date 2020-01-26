#include "Entity.h"
#include "EntityManager.h"

Entity::Entity()
	: EntityID(InvalidID)
{
}

Entity::Entity(uint32 EntityID)
	: EntityID(EntityID)
{
}

uint32 Entity::GetEntityID() const
{
	return EntityID;
}

Entity::operator bool() const
{
	return EntityID != InvalidID;
}