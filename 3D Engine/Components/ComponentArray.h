#pragma once
#include "ComponentSystems/ComponentSystem.h"

class IComponentArray
{
public:
	virtual void RemoveComponent(const uint64 EntityID) = 0;
	virtual std::shared_ptr<void> CopyComponent(const uint64 EntityID) = 0;
	virtual void AddComponent(const uint64 EntityID, std::shared_ptr<void> Data) = 0;
	virtual bool HasComponent(const uint64 EntityID) = 0;
	void QueueEntityForRenderUpdate(const uint64 EntityID);
};

template<typename TComponent>
class ComponentArray : public IComponentArray
{
public:
	ComponentArray()
	{
		GComponentSystemManager.AddComponentArray(*this);
	}

	template<typename ...Args>
	TComponent& AddComponent(const uint64 EntityID, Args&& ...InArgs)
	{
		if constexpr (TComponent::bNeedsRenderUpdate)
		{
			QueueEntityForRenderUpdate(EntityID);
		}

		Components.emplace(EntityID, TComponent(std::forward<Args>(InArgs)...));
		return Components[EntityID];
	}

	void AddComponent(const uint64 EntityID, std::shared_ptr<void> Data)
	{
		if constexpr (TComponent::bNeedsRenderUpdate)
		{
			QueueEntityForRenderUpdate(EntityID);
		}

		Components.emplace(EntityID, *std::static_pointer_cast<TComponent>(Data));
	}

	TComponent& GetComponent(const uint64 EntityID)
	{
		return Components[EntityID];
	}

	virtual std::shared_ptr<void> CopyComponent(const uint64 EntityID) final
	{
		return std::make_shared<TComponent>(GetComponent(EntityID));
	}

	virtual bool HasComponent(const uint64 EntityID) final
	{
		return Contains(Components, EntityID);
	}

	virtual void RemoveComponent(const uint64 EntityID) final
	{
		Components.erase(EntityID);
	}

	static ComponentArray<TComponent>& Get()
	{
		static ComponentArray<TComponent> ComponentArray;
		return ComponentArray;
	}

private:
	// @todo Object pool for components
	Map<uint64, TComponent> Components;
};