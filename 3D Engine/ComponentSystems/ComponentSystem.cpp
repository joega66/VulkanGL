#include "ComponentSystem.h"
#include <Components/ComponentArray.h>
#include <Components/Entity.h>
#include <Renderer/Scene.h>

ComponentSystemManager GComponentSystemManager;

void ComponentSystemManager::AddComponentSystem(ComponentSystem& ComponentSystem)
{
	ComponentSystems.push_back(ComponentSystem);
}

void ComponentSystemManager::AddComponentArray(IComponentArray& ComponentArray)
{
	ComponentArrays.push_back(ComponentArray);
}

void ComponentSystemManager::UpdateSystems()
{
	for (auto& ComponentSystem : ComponentSystems)
	{
		ComponentSystem.get().Update();
	}
}

void ComponentSystemManager::DestroyEntity(uint64 EntityID)
{
	for (auto& ComponentArray : ComponentArrays)
	{
		ComponentArray.get().RemoveComponent(EntityID);
	}
}