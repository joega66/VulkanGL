#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr uint64 InvalidID = std::numeric_limits<uint64>::max();

	Entity();

	Entity(uint64 EntityID);

	bool operator==(const Entity& Entity)
	{
		return EntityID == Entity.GetEntityID();
	}

	bool operator!=(const Entity& Entity)
	{
		return EntityID != Entity.GetEntityID();
	}

	template<typename TComponent, typename ...Args>
	TComponent& AddComponent(Args&& ...InArgs)
	{
		return ComponentArray<TComponent>::Get().AddComponent(EntityID, std::forward<Args>(InArgs)...);
	}

	template<typename TComponent>
	TComponent& GetComponent()
	{
		return ComponentArray<TComponent>::Get().GetComponent(EntityID);
	}

	template<typename TComponent>
	bool HasComponent()
	{
		return ComponentArray<TComponent>::Get().HasComponent(EntityID);
	}

	template<typename TComponent>
	void RemoveComponent()
	{
		ComponentArray<TComponent>::Get().RemoveComponent(EntityID);
	}

	void DestroyEntity();

	uint64 GetEntityID() const;

private:
	uint64 EntityID;
};

class EntityManager
{
public:
	EntityManager();

	Entity CreatePrefab(const std::string& Name);
	Entity CreateEntity();
	Entity CreateFromPrefab(Entity Prefab);
	Entity CreateFromPrefab(const std::string& Name);

	void DestroyEntity(const uint64 EntityID);

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
	friend class Entity;

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

	uint64 NextEntityID = 0;
	HashTable<std::string, Entity> Prefabs;
	HashTable<uint64, std::string> PrefabNames;
	std::vector<Entity> Entities;
};

extern EntityManager GEntityManager;