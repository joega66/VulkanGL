#include "EntityManager.h"
#include <Components/CTransform.h>
#include <Components/CRenderer.h>

EntityManager GEntityManager;

EntityManager::EntityManager()
{
}

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	check(!Contains(Prefabs, Name), "Prefab %s already exists.", Name.c_str());
	Prefabs.emplace(Name, Entity{ NextEntityID++ });
	Entity Prefab = GetValue(Prefabs, Name);
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	AddComponent<CTransform>(Prefab);
	AddComponent<CRenderer>(Prefab);
	return Prefab;
}

Entity EntityManager::CreateEntity()
{
	Entity NewEntity = Entities.emplace_back(Entity{ NextEntityID++ });
	AddComponent<CTransform>(NewEntity);
	AddComponent<CRenderer>(NewEntity);
	return NewEntity;
}

Entity EntityManager::Clone(Entity& Prefab)
{
	Entity Entity = CreateEntity();
	
	for (auto& ComponentArrayEntry : ComponentArrays)
	{
		auto ComponentArray = ComponentArrayEntry.second.get();
		if (ComponentArray->HasComponent(Prefab))
		{
			auto Component = ComponentArray->CopyComponent(Prefab);
			ComponentArray->AddComponent(Entity, std::move(Component));
		}
	}

	return Entity;
}

Entity EntityManager::CreateFromPrefab(const std::string& Name)
{
	check(Contains(Prefabs, Name), "No Prefab named %s", Name.c_str());
	return Clone(Prefabs[Name]);
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