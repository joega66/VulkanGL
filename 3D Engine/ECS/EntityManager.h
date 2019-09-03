#pragma once
#include "Entity.h"

class EntityManager
{
public:
	EntityManager();

	Entity CreatePrefab(const std::string& Name);

	Entity CreateEntity();

	Entity Clone(Entity& Prefab);

	Entity CreateFromPrefab(const std::string& Name);

	void Destroy(Entity& Entity);

	template<typename TComponent, typename ...Args>
	TComponent& AddComponent(Entity& Entity, Args&& ...InArgs)
	{
		auto Array = GetComponentArray<TComponent>();
		return Array->AddComponent(Entity, std::forward<Args>(InArgs)...);
	}

	template<typename TComponent>
	TComponent& GetComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<TComponent>();
		return Array->GetComponent(Entity);
	}

	template<typename TComponent>
	bool HasComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<TComponent>();
		return Array->HasComponent(Entity);
	}

	template<typename TComponent>
	void RemoveComponent(Entity& Entity)
	{
		auto Array = GetComponentArray<TComponent>();
		return Array->RemoveComponent(Entity);
	}

	template<typename ...TComponents>
	std::vector<Entity> GetEntities()
	{
		std::vector<Entity> EntitiesWithComponents;

		for (auto Entity : Entities)
		{
			if (EntityHasComponents<TComponents...>(Entity))
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

	template<typename T>
	bool EntityHasComponents(Entity& Entity)
	{
		return HasComponent<T>(Entity);
	}

	template<typename T1, typename T2, typename ...More>
	bool EntityHasComponents(Entity& Entity)
	{
		return EntityHasComponents<T1>(Entity) && EntityHasComponents<T2, More...>(Entity);
	}
};

extern EntityManager GEntityManager;

#include "ComponentArray.inl"