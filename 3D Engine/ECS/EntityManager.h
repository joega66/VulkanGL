#pragma once
#include "Entity.h"
#include "Component.h"

class EntityIterator
{
	friend class EntityManager;
	EntityIterator(std::vector<Entity>& entities, const std::vector<bool>& entityStatus);

public:
	Entity& Next();
	bool End();

private:
	uint32 _CurrIndex = 0;
	std::vector<Entity>& _Entities;
	const std::vector<bool>& _EntityStatus;
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
	Entity CreatePrefab(const std::string& name);

	/** Create an empty entity. */
	Entity CreateEntity(const std::string& name = "");

	/** Destroy the entity and all associated components. */
	void Destroy(Entity& entity);

	template<typename ComponentType>
	ComponentType& AddComponent(Entity& entity, ComponentType&& componentData)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto componentArray = GetComponentArray<ComponentType>();
		return componentArray->AddComponent(entity, std::move(componentData));
	}

	template<typename ComponentType>
	ComponentType& GetComponent(Entity& entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto componentArray = GetComponentArray<ComponentType>();
		return componentArray->GetComponent(entity);
	}

	template<typename ComponentType>
	bool HasComponent(Entity& entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto componentArray = GetComponentArray<ComponentType>();
		return componentArray->HasComponent(entity);
	}

	template<typename ComponentType>
	void RemoveComponent(Entity& entity)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto componentArray = GetComponentArray<ComponentType>();
		return componentArray->RemoveComponent(entity);
	}

	template<typename ComponentType, typename ...ComponentArgs>
	ComponentType& AddSingletonComponent(ComponentArgs&& ...args)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);

		const std::size_t arrayIndex = _SingletonComponentsArray.size();

		_SingletonComponentsArray.push_back(nullptr);

		_SingletonComponentsArray[arrayIndex] = std::make_shared<ComponentType>(args...);

		_SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))] = arrayIndex;

		_ArrayIndexToSingletonType[arrayIndex] = std::type_index(typeid(ComponentType)).hash_code();

		return *std::static_pointer_cast<ComponentType>(_SingletonComponentsArray.back());
	}

	template<typename ComponentType>
	ComponentType& GetSingletonComponent()
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		const std::size_t arrayIndex = _SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))];
		return *std::static_pointer_cast<ComponentType>(_SingletonComponentsArray[arrayIndex]);
	}

	template<typename ComponentType>
	void RemoveSingletonComponent()
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		const std::size_t back = _SingletonComponentsArray.size() - 1;
		const std::size_t moveTo = _SingletonTypeToArrayIndex[std::type_index(typeid(ComponentType))];
		const std::size_t movedSingletonType = _ArrayIndexToSingletonType[back];

		if (_SingletonComponentsArray.size() > 1)
		{
			_SingletonComponentsArray[moveTo] = _SingletonComponentsArray.back();
			_SingletonTypeToArrayIndex[movedSingletonType] = moveTo;
			_ArrayIndexToSingletonType[moveTo] = movedSingletonType;
		}

		_SingletonComponentsArray.pop_back();

		_ArrayIndexToSingletonType.erase(back);

		_SingletonTypeToArrayIndex.erase(std::type_index(typeid(ComponentType)));
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

		std::vector<Entity> entitiesWithComponent;
		entitiesWithComponent.reserve(GetComponentArray<ComponentType>()->GetEntities().size());

		for (const auto& [entity, arrayIndex] : GetComponentArray<ComponentType>()->GetEntities())
		{
			entitiesWithComponent.push_back(entity);
		}

		return entitiesWithComponent;
	}

	/** Add a callback for when ComponentType is created. */
	template<typename ComponentType>
	void OnComponentCreated(ComponentEvent<ComponentType> componentEvent)
	{
		static_assert(std::is_base_of<Component, ComponentType>::value);
		auto componentArray = GetComponentArray<ComponentType>();
		componentArray->OnComponentCreated(componentEvent);
	}

	/** Create an entity iterator. */
	EntityIterator Iter();

	/** Whether the entity is a valid one. */
	inline bool IsValid(Entity entity) { return entity.GetEntityID() != Entity::_InvalidID && _EntityStatus[entity.GetEntityID()]; }

	/** Call component events. */
	void NotifyComponentEvents();

	/** Get the name of an entity. */
	inline const std::string& GetName(Entity& entity) { return _EntityNames[entity.GetEntityID()]; }

private:
	/** Map of prefab names to prefab entities. */
	std::unordered_map<std::string, Entity> _Prefabs;

	/** Map of entity id's to prefab names*/
	std::unordered_map<std::size_t, std::string> _PrefabNames;

	/** Entity pool. */
	std::vector<Entity> _Entities;

	/** Map of entity id to entity name. */
	std::unordered_map<std::size_t, std::string> _EntityNames;

	/** Whether the entity is dead or alive. */
	std::vector<bool> _EntityStatus;

	/** Dead entities for recycling Entity IDs. */
	std::list<Entity> _DeadEntities;

	/** Map of a component's type to its component array. */
	std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> _ComponentArrays;

	/** Array of singleton components. */
	std::vector<std::shared_ptr<void>> _SingletonComponentsArray;

	/** Singleton type index to its array index. */
	std::unordered_map<std::type_index, std::size_t> _SingletonTypeToArrayIndex;

	/** Maps singleton array index to singleton type index. */
	std::unordered_map<std::size_t, std::size_t> _ArrayIndexToSingletonType;

	template<typename ComponentType>
	ComponentArray<ComponentType>* GetComponentArray()
	{
		const std::type_index typeIndex = std::type_index(typeid(ComponentType));
		if (auto arrayIter = _ComponentArrays.find(typeIndex); arrayIter != _ComponentArrays.end())
		{
			return (ComponentArray<ComponentType>*)arrayIter->second.get();
		}
		else
		{
			_ComponentArrays[typeIndex] = std::make_unique<ComponentArray<ComponentType>>();
			return (ComponentArray<ComponentType>*)_ComponentArrays[typeIndex].get();
		}
	}

	template<typename ComponentType>
	bool EntityHasComponents(Entity& entity)
	{
		return HasComponent<ComponentType>(entity);
	}

	template<typename ComponentType1, typename ComponentType2, typename ...More>
	bool EntityHasComponents(Entity& entity)
	{
		return EntityHasComponents<ComponentType1>(entity) && EntityHasComponents<ComponentType2, More...>(entity);
	}
};

#include "ComponentArray.inl"