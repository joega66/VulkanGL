#include "Entity.h"
#include "EntityManager.h"

Entity::Entity()
	: EntityID(InvalidID)
{
}

Entity::Entity(std::size_t EntityID)
	: EntityID(EntityID)
{
}