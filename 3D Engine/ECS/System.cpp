#include "System.h"

void SystemsManager::Register(ISystem& System)
{
	Systems.push_back(System);
}

void SystemsManager::StartSystems(Scene& Scene)
{
	for (auto& System : Systems)
	{
		System.get().Start(Scene);
	}
}

void SystemsManager::UpdateSystems(Scene& Scene)
{
	for (auto& System : Systems)
	{
		System.get().Update(Scene);
	}
}

void SystemsManager::UpdateRenderSystems(SceneProxy& Scene)
{
	for (auto& RenderSystem : RenderSystems)
	{
		RenderSystem.get().Update(Scene);
	}
}