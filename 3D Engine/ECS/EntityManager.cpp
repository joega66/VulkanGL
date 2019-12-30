#include "EntityManager.h"
#include <Components/CTransform.h>
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

void EntityManager::InitDefaultComponents(Entity& Entity)
{
	AddComponent<CTransform>(Entity);
	AddComponent<CRenderer>(Entity);
}

bool EntityManager::IsVisible(Entity& Entity) 
{ 
	return GetComponent<CRenderer>(Entity).bVisible; 
}