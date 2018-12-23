#include "Entity.h"
#include "ComponentSystems/ComponentSystem.h"
#include "CTransform.h"

EntityManager GEntityManager;

Entity::Entity()
	: EntityID(InvalidID)
{
}

Entity::Entity(uint64 EntityID)
	: EntityID(EntityID)
{
}

void Entity::DestroyEntity()
{
	GComponentSystemManager.DestroyEntity(EntityID);
	GEntityManager.DestroyEntity(EntityID);
}

uint64 Entity::GetEntityID() const
{
	return EntityID;
}

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	Prefabs.emplace(Name, Entity{ NextEntityID++ });
	Entity Prefab = GetValue(Prefabs, Name);
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	Prefab.AddComponent<CTransform>();
	return Prefab;
}

Entity EntityManager::CreateEntity()
{
	Entity NewEntity = Entities.emplace_back(Entity{ NextEntityID++ });
	NewEntity.AddComponent<CTransform>();
	return NewEntity;
}

Entity EntityManager::CreateFromPrefab(Entity Prefab)
{
	Entity Entity = CreateEntity();
	auto& ComponentArrays = GComponentSystemManager.ComponentArrays;

	for (auto& ComponentArray : ComponentArrays)
	{
		if (ComponentArray.get().HasComponent(Prefab.GetEntityID()))
		{
			auto Component = ComponentArray.get().CopyComponent(Prefab.GetEntityID());
			ComponentArray.get().AddComponent(Entity.GetEntityID(), std::move(Component));
		}
	}

	return Entity;
}

Entity EntityManager::CreateFromPrefab(const std::string& Name)
{
	if (Contains(Prefabs, Name))
	{
		return CreateFromPrefab(GetValue(Prefabs, Name));
	}

	return Entity();
}

void EntityManager::DestroyEntity(const uint64 EntityID)
{
	Entities.erase(std::remove_if(Entities.begin(), Entities.end(), [&] (auto& Other)
	{
		return EntityID == Other.GetEntityID();
	}));
}

void EntityManager::QueueEntityForRenderUpdate(Entity Entity)
{
	// Prefabs are not updated by component systems
	if (!Contains(PrefabNames, Entity.GetEntityID()))
	{
		EntitiesForRenderUpdate.push_back(Entity);
	}
}