#pragma once
#include "ComponentArray.h"

// @todo Should also perform checks for invalid entities
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
	Entity CreatePrefab(const std::string& Name);
	Entity CreateEntity();
	Entity CreateFromPrefab(Entity Prefab);
	Entity CreateFromPrefab(const std::string& Name);

	void DestroyEntity(const uint64 EntityID);

	template<typename ...TComponents>
	std::vector<Entity> GetEntitiesForRenderUpdate()
	{
		std::vector<Entity> EntitiesWithComponents;

		for (auto Entity : EntitiesForRenderUpdate)
		{
			if (EntityHasComponents<TComponents...>(Entity))
			{
				EntitiesWithComponents.push_back(Entity);
			}
		}

		return EntitiesWithComponents;
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
	friend class ComponentSystemManager;
	friend class IComponentArray;
	friend class Entity;

	void QueueEntityForRenderUpdate(Entity Entity);

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

	// @todo Not ideal. Should use an object pool to store entities.
	uint64 NextEntityID = 0;
	Map<std::string, Entity> Prefabs;
	Map<uint64, std::string> PrefabNames;
	std::vector<Entity> Entities;
	std::vector<Entity> EntitiesForRenderUpdate;
};

extern EntityManager GEntityManager;