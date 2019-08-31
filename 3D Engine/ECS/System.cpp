#include "System.h"
#include <Components/ComponentArray.h>
#include <Components/Entity.h>
#include <Renderer/Scene.h>

class SystemsHerder SystemsHerder;

void SystemsHerder::Register(ISystem& System)
{
	Systems.push_back(System);
}

void SystemsHerder::AddComponentArray(IComponentArray& ComponentArray)
{
	ComponentArrays.push_back(ComponentArray);
}

void SystemsHerder::StartSystems()
{
	for (auto& System : Systems)
	{
		System.get().Start();
	}
}

void SystemsHerder::UpdateSystems()
{
	for (auto& System : Systems)
	{
		System.get().Update();
	}
}

void SystemsHerder::DestroyEntity(Entity& Entity)
{
	for (auto& ComponentArray : ComponentArrays)
	{
		ComponentArray.get().RemoveComponent(Entity);
	}
}