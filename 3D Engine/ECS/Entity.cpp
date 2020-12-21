#include "Entity.h"
#include "EntityManager.h"

Entity::Entity()
	: _EntityID(_InvalidID)
{
}

Entity::Entity(std::size_t entityID)
	: _EntityID(entityID)
{
}