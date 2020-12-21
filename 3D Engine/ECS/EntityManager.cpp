#include "EntityManager.h"
#include <Components/Transform.h>

Entity EntityManager::CreatePrefab(const std::string& name)
{
	check(!_Prefabs.contains(name), "Prefab %s already exists.", name.c_str());

	_Prefabs.emplace(name, CreateEntity());

	Entity prefab = _Prefabs[name];

	_PrefabNames.emplace(prefab.GetEntityID(), name);

	return prefab;
}

Entity EntityManager::CreateEntity(const std::string& name)
{
	Entity entity = [&] ()
	{
		if (!_DeadEntities.empty())
		{
			auto entity = _DeadEntities.front();

			_DeadEntities.pop_front();

			_EntityStatus[entity.GetEntityID()] = true;

			return entity;
		}
		else
		{
			_EntityStatus.push_back(true);

			return _Entities.emplace_back(Entity( _Entities.size() ));
		}
	}();
	
	// Add components every entity should probably have...
	AddComponent(entity, Transform(*this, entity));

	_EntityNames[entity.GetEntityID()] = name.empty() ? "Entity" + std::to_string(entity.GetEntityID()) : name;

	return entity;
}

void EntityManager::Destroy(Entity& entity)
{
	for (auto& [type, componentArray] : _ComponentArrays)
	{
		if (componentArray.get()->HasComponent(entity))
		{
			componentArray.get()->RemoveComponent(entity);
		}
	}

	_DeadEntities.push_back(entity);

	_EntityStatus[entity.GetEntityID()] = false;

	_EntityNames.erase(entity.GetEntityID());
}

EntityIterator EntityManager::Iter()
{
	return EntityIterator(_Entities, _EntityStatus);
}

void EntityManager::NotifyComponentEvents()
{
	for (auto& componentArrayEntry : _ComponentArrays)
	{
		auto componentArray = componentArrayEntry.second.get();
		componentArray->NotifyOnComponentCreatedEvents();
	}
}

EntityIterator::EntityIterator(std::vector<Entity>& entities, const std::vector<bool>& entityStatus)
	: _Entities(entities), _EntityStatus(entityStatus)
{
}

Entity& EntityIterator::Next()
{
	return _Entities[_CurrIndex++];
}

bool EntityIterator::End()
{
	while (_CurrIndex != _Entities.size() && _EntityStatus[_CurrIndex] == false)
	{
		_CurrIndex++;
	}

	return _CurrIndex == _Entities.size();
}
