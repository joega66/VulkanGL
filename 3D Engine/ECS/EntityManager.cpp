#include "EntityManager.h"
#include <Components/Transform.h>

Entity EntityManager::CreatePrefab(const std::string& Name)
{
	check(!Prefabs.contains(Name), "Prefab %s already exists.", Name.c_str());
	Prefabs.emplace(Name, CreateEntity());
	Entity Prefab = Prefabs[Name];
	PrefabNames.emplace(Prefab.GetEntityID(), Name);
	return Prefab;
}

Entity EntityManager::CreateEntity(const std::string& Name)
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

	EntityNames[NewEntity.GetEntityID()] = Name.empty() ? "Entity" + std::to_string(NewEntity.GetEntityID()) : Name;

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

	EntityNames.erase(Entity.GetEntityID());
}

EntityIterator EntityManager::Iter()
{
	return EntityIterator(Entities, EntityStatus);
}

void EntityManager::NotifyComponentEvents()
{
	for (auto& ComponentArrayEntry : ComponentArrays)
	{
		auto ComponentArray = ComponentArrayEntry.second.get();
		ComponentArray->NotifyComponentCreatedEvents();
	}
}

EntityIterator::EntityIterator(std::vector<Entity>& Entities, const std::vector<bool>& EntityStatus)
	: Entities(Entities), EntityStatus(EntityStatus)
{
}

Entity& EntityIterator::Next()
{
	return Entities[CurrIndex++];
}

bool EntityIterator::End()
{
	while (CurrIndex != Entities.size() && EntityStatus[CurrIndex] == false)
	{
		CurrIndex++;
	}

	return CurrIndex == Entities.size();
}
