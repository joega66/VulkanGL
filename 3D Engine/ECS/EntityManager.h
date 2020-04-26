#pragma once
#include "Entity.h"
#include "Component.h"

class EntityIterator
{
	friend class EntityManager;
	EntityIterator(std::vector<Entity>& Entities, const std::vector<bool>& EntityStatus);

public:
	Entity& Next();
	bool End();

private:
	uint32 CurrIndex = 0;
	std::vector<Entity>& Entities;
	const std::vector<bool>& EntityStatus;
};

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
	Entity CreateEntity(const std::string& Name = "");

	/** Destroy the entity and all associated components. */
	void Destroy(Entity& Entity);

	template<typename ComponentType>
	ComponentType& AddComponent(Entity& Entity, ComponentType&& ComponentData)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto Array = GetComponentArray<ComponentType>();
		return Array->AddComponent(Entity, std::move(ComponentData));
	}

	template<typename ComponentType>
	ComponentType& GetComponent(Entity& Entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto Array = GetComponentArray<ComponentType>();
		return Array->GetComponent(Entity);
	}

	template<typename ComponentType>
	bool HasComponent(Entity& Entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto Array = GetComponentArray<ComponentType>();
		return Array->HasComponent(Entity);
	}

	template<typename ComponentType>
	void RemoveComponent(Entity& Entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto Array = GetComponentArray<ComponentType>();
		return Array->RemoveComponent(Entity);
	}

	template<typename ComponentType, typename ...ComponentArgs>
	ComponentType& AddSingletonComponent(ComponentArgs&& ...Args)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);

		std::size_t ArrayIndex;

		if (FreeSingletonComponentArrayIndices.size())
		{
			ArrayIndex = FreeSingletonComponentArrayIndices.front();
			FreeSingletonComponentArrayIndices.pop_front();
		}
		else
		{
			SingletonComponentsArray.push_back(nullptr);
			ArrayIndex = SingletonComponentsArray.size() - 1;
		}

		SingletonComponentsArray[ArrayIndex] = std::make_shared<ComponentType>(Args...);
		SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))] = ArrayIndex;
		return *std::static_pointer_cast<ComponentType>(SingletonComponentsArray.back());
	}

	template<typename ComponentType>
	ComponentType& GetSingletonComponent()
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		const std::size_t ArrayIndex = SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))];
		return *std::static_pointer_cast<ComponentType>(SingletonComponentsArray[ArrayIndex]);
	}

	template<typename ComponentType>
	void RemoveSingletonComponent()
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		const uint32 ArrayIndex = SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))];
		SingletonComponentsArray[ArrayIndex] = nullptr;
		FreeSingletonComponentArrayIndices.push_back(ArrayIndex);
		SingletonTypeToArrayIndex.erase(std::type_index(typeid(ComponentType)));
	}

	/** 
	  * GetEntities
	  * @return Entities with types ComponentTypes.
	  * @todo Allow tuples.
	  */
	template<typename ComponentType>
	std::vector<Entity> GetEntities()
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);

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
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto Array = GetComponentArray<ComponentType>();
		Array->NewComponentCallback(ComponentCallback);
	}

	/** Create an entity iterator. */
	EntityIterator Iter();

	/** Whether the entity is a valid one. */
	inline bool IsValid(Entity Entity) { return Entity.GetEntityID() != Entity::InvalidID && EntityStatus[Entity.GetEntityID()]; }

	/** Call component events. */
	void NotifyComponentEvents();

	/** Get the name of an entity. */
	inline const std::string& GetName(Entity& Entity) { return EntityNames[Entity.GetEntityID()]; }

private:
	/** Map of prefab names to prefab entities. */
	std::unordered_map<std::string, Entity> Prefabs;

	/** Map of entity id's to prefab names*/
	std::unordered_map<uint32, std::string> PrefabNames;

	/** Entity pool. */
	std::vector<Entity> Entities;

	/** Map of entity id to entity name. */
	std::unordered_map<uint32, std::string> EntityNames;

	/** Whether the entity is dead or alive. */
	std::vector<bool> EntityStatus;

	/** Dead entities for recycling Entity IDs. */
	std::list<Entity> DeadEntities;

	/** Map of a component's type to its component array. */
	std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> ComponentArrays;

	/** Array of singleton components. */
	std::vector<std::shared_ptr<void>> SingletonComponentsArray;

	/** Singleton type index to its array index. */
	std::unordered_map<std::type_index, std::size_t> SingletonTypeToArrayIndex;

	/** Free indices in the singleton component array. */
	std::list<std::size_t> FreeSingletonComponentArrayIndices;

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