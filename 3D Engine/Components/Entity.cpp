#include "Entity.h"
#include "ComponentSystems/ComponentSystem.h"

EntityManager GEntityManager;

Entity::Entity(uint64 EntityID)
	: EntityID(EntityID)
{
}

void Entity::DestroyEntity()
{
	GComponentSystemManager.DestroyEntity(this);
	GEntityManager.DestroyEntity(this);
}

uint64 Entity::GetEntityID()
{
	return EntityID;
}

Entity& EntityManager::CreateEntity()
{
	Entity& NewEntity = Entities.emplace_back(Entity{ NextEntityID++ });
	return NewEntity;
}

void EntityManager::DestroyEntity(Entity* Entity)
{
	Entities.erase(std::remove_if(Entities.begin(), Entities.end(), [&] (auto& Other)
	{
		return Entity == &Other;
	}));
}

void EntityManager::QueueEntityThatNeedsRenderUpdate(Entity* Entity)
{
	EntitiesThatNeedRenderUpdate.push_back(Entity);
}