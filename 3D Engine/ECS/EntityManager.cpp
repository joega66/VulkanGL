#include "EntityManager.h"
#include <Components/Transform.h>
#include <Components/CRenderer.h>

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	check(!Contains(Prefabs, Name), "Prefab %s already exists.", Name.c_str());
	Prefabs.emplace(Name, Entity{ NextEntityID++ });
	Entity Prefab = GetValue(Prefabs, Name);
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	InitDefaultComponents(Prefab);
	return Prefab;
}

Entity EntityManager::CreateEntity()
{
	Entity NewEntity = Entities.emplace_back(Entity{ NextEntityID++ });
	InitDefaultComponents(NewEntity);
	return NewEntity;
}

Entity EntityManager::CreateEntity(const std::string& Name)
{
	check(Contains(Prefabs, Name), "No Prefab named %s", Name.c_str());
	return Clone(Prefabs[Name]);
}

Entity EntityManager::Clone(Entity& Other)
{
	Entity Entity = CreateEntity();
	
	for (auto& ComponentArrayEntry : ComponentArrays)
	{
		auto ComponentArray = ComponentArrayEntry.second.get();
		if (ComponentArray->HasComponent(Other))
		{
			ComponentArray->CopyComponent(Entity, Other);
		}
	}

	return Entity;
}

void EntityManager::Destroy(Entity& Entity)
{
	for (auto& ComponentArray : ComponentArrays)
	{
		ComponentArray.second.get()->RemoveComponent(Entity);
	}

	Entities.erase(std::remove_if(Entities.begin(), Entities.end(), [&] (auto& Other)
	{
		return Entity == Other;
	}));
}

void EntityManager::NotifyComponentEvents()
{
	for (auto& ComponentArrayEntry : ComponentArrays)
	{
		auto ComponentArray = ComponentArrayEntry.second.get();
		ComponentArray->NotifyComponentCreatedEvents();
	}
}

void EntityManager::InitDefaultComponents(Entity& Entity)
{
	AddComponent<Transform>(Entity);
	AddComponent<CRenderer>(Entity);
}