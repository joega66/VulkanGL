#pragma once
#include "Entity.h"

/** The EntityManager stores entities and performs component operations (Add, Get, Has, Remove) */
class EntityManager
{
	/** Only Scene can construct the EntityManager. */
	friend class Scene;
	EntityManager() = default;

public:
	EntityManager(const EntityManager&) = delete;

	EntityManager& operator=(const EntityManager&) = delete;

	/** Create a prefab entity. */
	Entity CreatePrefab(const std::string& Name);

	/** Create an empty entity. */
	Entity CreateEntity();

	/** Create an entity from a prefab. */
	Entity CreateEntity(const std::string& Name);

	/** Clone an entity. */
	Entity Clone(Entity& Other);

	/** Destroy the entity and all associated components. */
	void Destroy(Entity& Entity);

	template<typename ComponentType, typename ...Args>
	ComponentType& AddComponent(Entity& Entity, Args&& ...InArgs)
	{
		auto Array = GetComponentArray<ComponentType>();
		ComponentType& Component = Array->AddComponent(Entity, InArgs...);
		Array->NotifyComponentCreated(Entity, Component);
		return Component;
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

	/** GetEntities
	  * @return Entities with types ComponentTypes.
	  * Entities are returned in sorted order for faster ranges operations.
	  */
	template<typename ...ComponentTypes>
	std::vector<Entity> GetEntities()
	{
		std::vector<Entity> EntitiesWithComponents;

		for (auto Entity : Entities)
		{
			if (EntityHasComponents<ComponentTypes...>(Entity))
			{
				EntitiesWithComponents.push_back(Entity);
			}
		}

		return EntitiesWithComponents;
	}

	/** GetVisibleEntities
	  * @return Entities with ComponentTypes which are not hidden.
	  * Entities are returned in sorted order for faster ranges operations.
	  */
	template<typename ...ComponentTypes>
	std::vector<Entity> GetVisibleEntities()
	{
		std::vector<Entity> EntitiesWithComponents;

		for (auto Entity : Entities)
		{
			if (IsVisible(Entity) &&
				EntityHasComponents<ComponentTypes...>(Entity))
			{
				EntitiesWithComponents.push_back(Entity);
			}
		}

		return EntitiesWithComponents;
	}

	/** Add a callback for when ComponentType is created. */
	template<typename ComponentType>
	void NewComponentCallback(ComponentCallback<ComponentType> ComponentCallback)
	{
		auto Array = GetComponentArray<ComponentType>();
		Array->NewComponentCallback(ComponentCallback);
	}

private:
	/** Next entity id to be allocated. */
	uint64 NextEntityID = 0;

	/** Map of prefab names to prefab entities. */
	HashTable<std::string, Entity> Prefabs;

	/** Map of entity id's to prefab names*/
	HashTable<uint64, std::string> PrefabNames;

	/** Currently active entities in the scene. */
	std::vector<Entity> Entities;

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

	void InitDefaultComponents(Entity& Entity);

	bool IsVisible(Entity& Entity);
};

#include "ComponentArray.inl"