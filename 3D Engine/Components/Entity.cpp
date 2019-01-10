#include "Entity.h"
#include <ComponentSystems/ComponentSystem.h>
#include "CTransform.h"
#include "CRenderer.h"

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
	GEntityManager.DestroyEntity(*this);
}

uint64 Entity::GetEntityID() const
{
	return EntityID;
}

Entity::operator bool() const
{
	return EntityID != InvalidID;
}

EntityManager::EntityManager()
{
}

static void Templatize(Entity Entity)
{
	Entity.AddComponent<CTransform>();
	Entity.AddComponent<CRenderer>();
}

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	check(!Contains(Prefabs, Name), "Prefab %s already exists.", Name.c_str());
	Prefabs.emplace(Name, Entity{ NextEntityID++ });
	Entity Prefab = GetValue(Prefabs, Name);
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	Templatize(Prefab);
	return Prefab;
}

Entity EntityManager::CreateEntity()
{
	Entity NewEntity = Entities.emplace_back(Entity{ NextEntityID++ });
	Templatize(NewEntity);
	return NewEntity;
}

Entity EntityManager::CreateFromPrefab(Entity Prefab)
{
	Entity Entity = CreateEntity();
	auto& ComponentArrays = GComponentSystemManager.ComponentArrays;

	for (auto& ComponentArray : ComponentArrays)
	{
		if (ComponentArray.get().HasComponent(Prefab))
		{
			auto Component = ComponentArray.get().CopyComponent(Prefab);
			ComponentArray.get().AddComponent(Entity, std::move(Component));
		}
	}

	return Entity;
}

Entity EntityManager::CreateFromPrefab(const std::string& Name)
{
	check(Contains(Prefabs, Name), "No Prefab named %s", Name.c_str());
	return CreateFromPrefab(GetValue(Prefabs, Name));
}

void EntityManager::DestroyEntity(Entity& Entity)
{
	GComponentSystemManager.DestroyEntity(Entity);

	Entities.erase(std::remove_if(Entities.begin(), Entities.end(), [&] (auto& Other)
	{
		return Entity == Other;
	}));
}