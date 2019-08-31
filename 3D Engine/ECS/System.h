#pragma once
#include <Platform/Platform.h>
#include <typeindex>

class Entity;

class IComponentArray;

template<typename TComponent>
class ComponentArray;

class ISystem
{
public:
	virtual void Start() {}

	virtual void Update() {}

	virtual void OnRemove(std::type_index Type, Entity& Entity) {}

	template<typename TComponent>
	void Listen()
	{
		ComponentArray<TComponent>::Get().OnRemoveListener(*this);
	}
};

class SystemsHerder
{
private:
	std::vector<std::reference_wrapper<ISystem>> Systems;
	std::vector<std::reference_wrapper<IComponentArray>> ComponentArrays;

	friend class EntityManager;

	void DestroyEntity(Entity& Entity);

	template<typename TComponent>
	friend class ComponentArray;

	void AddComponentArray(IComponentArray& ComponentArray);

	friend class CoreEngine;

	void Register(ISystem& ISystem);

	void StartSystems();

	void UpdateSystems();
};

extern class SystemsHerder SystemsHerder;

// @todo-joe Hide access of RegisterSystem()
#define SYSTEM(SystemName) \
	public: \
	friend class ComponentSystemManager; \
	SystemName& operator=(const SystemName&) = delete; \
	SystemName(const SystemName&) = delete; \
	SystemName() {} \