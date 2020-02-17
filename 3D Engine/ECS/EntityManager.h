#pragma once
#include "Entity.h"

/** The EntityManager stores entities and performs component operations (Add, Get, Has, Remove) */
class EntityManager
{
	/** Only Engine can construct the EntityManager. */
	friend class Engine;
	EntityManager() = default;

public:
	EntityManager(const EntityManager&) = delete;

	EntityManager& operator=(const EntityManager&) = delete;

	/** Create a prefab entity. */
	Entity CreatePrefab(const std::string& Name);

	/** Create an empty entity. */
	Entity CreateEntity();

	/** Destroy the entity and all associated components. */
	void Destroy(Entity& Entity);

	template<typename ComponentType>
	ComponentType& AddComponent(Entity& Entity, ComponentType&& Component)
	{
		auto Array = GetComponentArray<ComponentType>();
		return Array->AddComponent(Entity, std::move(Component));
	}

	template<typename ComponentType>
	ComponentType& GetComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<ComponentType>();
		return Array->GetComponent(Entity);
	}

	template<typename ComponentType>
	bool HasComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<ComponentType>();
		return Array->HasComponent(Entity);
	}

	template<typename ComponentType>
	void RemoveComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<ComponentType>();
		return Array->RemoveComponent(Entity);
	}

	/** 
	  * GetEntities
	  * @return Entities with types ComponentTypes.
	  * @todo Allow tuples.
	  */
	template<typename ComponentType>
	std::vector<Entity> GetEntities()
	{
		std::vector<Entity> EntitiesWithComponent;
		EntitiesWithComponent.reserve(GetComponentArray<ComponentType>()->GetEntities().size());

		for (const auto& [Entity, ArrayIndex] : GetComponentArray<ComponentType>()->GetEntities())
		{
			EntitiesWithComponent.push_back(Entity);
		}

		return EntitiesWithComponent;
	}

	/** Add a callback for when ComponentType is created. */
	template<typename ComponentType>
	void NewComponentCallback(ComponentCallback<ComponentType> ComponentCallback)
	{
		auto Array = GetComponentArray<ComponentType>();
		Array->NewComponentCallback(ComponentCallback);
	}

	/** Whether the entity is a valid one. */
	inline bool IsValid(Entity Entity) { return Entity.GetEntityID() != Entity::InvalidID && EntityStatus[Entity.GetEntityID()]; }

	/** Call component events. */
	void NotifyComponentEvents();

private:
	/** Next entity id to be allocated. */
	uint32 NextEntityID = 0;

	/** Map of prefab names to prefab entities. */
	HashTable<std::string, Entity> Prefabs;

	/** Map of entity id's to prefab names*/
	HashTable<uint32, std::string> PrefabNames;

	/** Entity pool. */
	std::vector<Entity> Entities;

	/** Whether the entity is dead or alive. */
	std::vector<bool> EntityStatus;

	/** Dead entities for recycling Entity IDs. */
	std::list<Entity> DeadEntities;

	/** Map of a component's type to its component array. */
	HashTable<std::type_index, std::unique_ptr<IComponentArray>> ComponentArrays;

	template<typename ComponentType>
	ComponentArray<ComponentType>* GetComponentArray()
	{
		const std::type_index TypeIndex = std::type_index(typeid(ComponentType));
		if (auto ArrayIter = ComponentArrays.find(TypeIndex); ArrayIter != ComponentArrays.end())
		{
			return (ComponentArray<ComponentType>*)ArrayIter->second.get();
		}
		else
		{
			ComponentArrays[TypeIndex] = std::make_unique<ComponentArray<ComponentType>>();
			return (ComponentArray<ComponentType>*)ComponentArrays[TypeIndex].get();
		}
	}

	template<typename ComponentType>
	bool EntityHasComponents(Entity& Entity)
	{
		return HasComponent<ComponentType>(Entity);
	}

	template<typename ComponentType1, typename ComponentType2, typename ...More>
	bool EntityHasComponents(Entity& Entity)
	{
		return EntityHasComponents<ComponentType1>(Entity) && EntityHasComponents<ComponentType2, More...>(Entity);
	}
};

#include "ComponentArray.inl"