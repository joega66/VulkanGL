#pragma once
#include "ComponentArray.h"

class Entity
{
public:
	static constexpr uint64 InvalidID = std::numeric_limits<uint64>::max();

	Entity(uint64 EntityID);

	template<typename TComponent, typename ...Args>
	TComponent& AddComponent(Args&& ...InArgs)
	{
		if constexpr (TComponent::bNeedsRenderUpdate)
		{
			QueueEntityForRenderUpdate();
		}
		return ComponentArray<TComponent>::Get().AddComponent(this, std::forward<Args>(InArgs)...);
	}

	template<typename TComponent>
	TComponent& GetComponent()
	{
		return ComponentArray<TComponent>::Get().GetComponent(this);
	}

	template<typename TComponent>
	bool HasComponent()
	{
		return ComponentArray<TComponent>::Get().HasComponent(this);
	}

	template<typename TComponent>
	void RemoveComponent()
	{
		ComponentArray<TComponent>::Get().RemoveComponent(this);
	}

	void DestroyEntity();

	uint64 GetEntityID();

	void QueueEntityForRenderUpdate();

private:
	uint64 EntityID;
};

class EntityManager
{
public:
	Entity& CreateEntity();
	void DestroyEntity(Entity* Entity);

	template<typename ...TComponents>
	std::vector<Entity*> GetEntitiesThatNeedRenderUpdate()
	{
		std::vector<Entity*> EntitiesWithComponents;

		for (auto Entity : EntitiesThatNeedRenderUpdate)
		{
			if (EntityHasComponents<TComponents...>(*Entity))
			{
				EntitiesWithComponents.push_back(Entity);
			}
		}

		return EntitiesWithComponents;
	}

	template<typename ...TComponents>
	std::vector<Entity*> GetEntities()
	{
		std::vector<Entity*> EntitiesWithComponents;
		
		for (auto& Entity : Entities)
		{
			if (EntityHasComponents<TComponents...>(Entity))
			{
				EntitiesWithComponents.push_back(&Entity);
			}
		}

		return EntitiesWithComponents;
	}

private:
	friend class ComponentSystemManager;
	friend class Entity;

	void QueueEntityThatNeedsRenderUpdate(Entity* Entity);

	template<typename T>
	static bool EntityHasComponents(Entity& Entity)
	{
		return Entity.HasComponent<T>();
	}

	template<typename T1, typename T2, typename ...More>
	static bool EntityHasComponents(Entity& Entity)
	{
		return EntityHasComponents<T1>(Entity) && EntityHasComponents<T2, More...>(Entity);
	}

	// @todo Not ideal. Should use an object pool to store entities.
	uint64 NextEntityID = 0;
	std::list<Entity> Entities;
	std::list<Entity*> EntitiesThatNeedRenderUpdate;
};

extern EntityManager GEntityManager;