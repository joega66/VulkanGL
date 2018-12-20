#include "ComponentSystem.h"
#include "Components/ComponentArray.h"
#include "Components/Entity.h"
#include "Renderer/Scene.h"

ComponentSystemManager GComponentSystemManager;

void ComponentSystemManager::AddComponentSystem(ComponentSystem* ComponentSystem)
{
	ComponentSystems.push_back(ComponentSystem);
}

void ComponentSystemManager::AddRenderSystem(ComponentSystem* RenderSystem)
{
	RenderSystems.push_back(RenderSystem);
}

void ComponentSystemManager::AddComponentArray(IComponentArray* ComponentArray)
{
	ComponentArrays.push_back(ComponentArray);
}

void ComponentSystemManager::UpdateSystems(Scene* Scene)
{
	for (auto ComponentSystem : ComponentSystems)
	{
		ComponentSystem->Update(Scene);
	}

	for (auto RenderSystem : RenderSystems)
	{
		RenderSystem->RenderUpdate(Scene);
	}

	GEntityManager.EntitiesThatNeedRenderUpdate.clear();
}

void ComponentSystemManager::DestroyEntity(Entity* Entity)
{
	for (auto ComponentArray : ComponentArrays)
	{
		ComponentArray->RemoveComponent(Entity);
	}
}