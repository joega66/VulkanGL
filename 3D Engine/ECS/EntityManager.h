#pragma once
#include "Entity.h"

class EntityManager
{
	friend class Scene;
	EntityManager() = default;

public:
	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;

	Entity CreatePrefab(const std::string& Name);

	Entity CreateEntity();

	Entity Clone(Entity& Other);

	Entity CreateFromPrefab(const std::string& Name);

	void Destroy(Entity& Entity);

	template<typename ComponentType, typename ...Args>
	ComponentType& AddComponent(Entity& Entity, Args&& ...InArgs)
	{
		auto Array = GetComponentArray<ComponentType>();
		return Array->AddComponent(Entity, std::forward<Args>(InArgs)...);
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

private:
	uint64 NextEntityID = 0;
	HashTable<std::string, Entity> Prefabs;
	HashTable<uint64, std::string> PrefabNames;
	std::vector<Entity> Entities;
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
};

#include "ComponentArray.inl"