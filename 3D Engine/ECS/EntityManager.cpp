#include "EntityManager.h"
#include <Components/Transform.h>

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	check(!Contains(Prefabs, Name), "Prefab %s already exists.", Name.c_str());
	Prefabs.emplace(Name, CreateEntity());
	Entity Prefab = GetValue(Prefabs, Name);
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	return Prefab;
}

Entity EntityManager::CreateEntity()
{
	Entity NewEntity = [&] ()
	{
		if (!DeadEntities.empty())
		{
			auto Entity = DeadEntities.front();
			DeadEntities.pop_front();
			EntityStatus[Entity.GetEntityID()] = true;
			return Entity;
		}
		else
		{
			EntityStatus.push_back(true);
			return Entities.emplace_back(Entity{ static_cast<uint32>(Entities.size()) }); // @todo Use 64-bit entity id handles.
		}
	}();
	
	// Add components every entity should probably have...
	AddComponent(NewEntity, Transform(*this, NewEntity));

	return NewEntity;
}

void EntityManager::Destroy(Entity& Entity)
{
	for (auto& [Type, ComponentArray] : ComponentArrays)
	{
		if (ComponentArray.get()->HasComponent(Entity))
		{
			ComponentArray.get()->RemoveComponent(Entity);
		}
	}

	DeadEntities.push_back(Entity);

	EntityStatus[Entity.GetEntityID()] = false;
}

void EntityManager::NotifyComponentEvents()
{
	for (auto& ComponentArrayEntry : ComponentArrays)
	{
		auto ComponentArray = ComponentArrayEntry.second.get();
		ComponentArray->NotifyComponentCreatedEvents();
	}
}