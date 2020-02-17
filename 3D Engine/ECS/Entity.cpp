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