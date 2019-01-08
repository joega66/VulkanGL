#pragma once
#include <ComponentSystems/ComponentSystem.h>

class IComponentArray
{
public:
	virtual void RemoveComponent(uint64 EntityID) = 0;
	virtual std::shared_ptr<void> CopyComponent(const uint64 EntityID) = 0;
	virtual void AddComponent(const uint64 EntityID, std::shared_ptr<void> Data) = 0;
	virtual bool HasComponent(const uint64 EntityID) = 0;
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
		Components.emplace(EntityID, std::move(TComponent(std::forward<Args>(InArgs)...)));
		return Components[EntityID];
	}

	void AddComponent(const uint64 EntityID, std::shared_ptr<void> Data)
	{
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

	virtual void RemoveComponent(uint64 EntityID) final
	{
		Components.erase(EntityID);
		Entity Entity{ EntityID };
		for (auto& ComponentSystem : OnRemoveListeners)
		{
			ComponentSystem.get().OnRemove(std::type_index(typeid(TComponent)), Entity);
		}
	}

	void OnRemoveListener(ComponentSystem& ComponentSystem)
	{
		OnRemoveListeners.push_back(ComponentSystem);
	}

	static ComponentArray<TComponent>& Get()
	{
		static ComponentArray<TComponent> ComponentArray;
		return ComponentArray;
	}

private:
	// @todo Object pool for components
	HashTable<uint64, TComponent> Components;
	std::vector<std::reference_wrapper<ComponentSystem>> OnRemoveListeners;
};