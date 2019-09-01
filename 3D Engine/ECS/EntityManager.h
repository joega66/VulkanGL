#pragma once
#include "Entity.h"

class EntityManager
{
public:
	EntityManager();

	Entity CreatePrefab(const std::string& Name);
	Entity CreateEntity();
	Entity CreateFromPrefab(Entity Prefab);
	Entity CreateFromPrefab(const std::string& Name);

	void DestroyEntity(Entity& Entity);

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
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;

	template<typename TComponent>
	friend class ComponentArray;
	void AddComponentArray(IComponentArray& ComponentArray);

	template<typename T>
	static bool EntityHasComponents(Entity Entity)
	{
		return Entity.HasComponent<T>();
	}

	template<typename T1, typename T2, typename ...More>
	static bool EntityHasComponents(Entity Entity)
	{
		return EntityHasComponents<T1>(Entity) && EntityHasComponents<T2, More...>(Entity);
	}
};

extern EntityManager GEntityManager;

#include "ComponentArray.inl"