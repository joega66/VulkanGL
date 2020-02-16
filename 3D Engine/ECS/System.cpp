#include "System.h"

void SystemsManager::Register(ISystem& System)
{
	Systems.push_back(System);
}

void SystemsManager::Register(IRenderSystem& System)
{
	RenderSystems.push_back(System);
}

void SystemsManager::StartSystems(class Engine& Engine)
{
	for (auto& System : Systems)
	{
		System.get().Start(Engine);
	}
}

void SystemsManager::UpdateSystems(class Engine& Engine)
{
	for (auto& System : Systems)
	{
		System.get().Update(Engine);
	}
}

void SystemsManager::StartRenderSystems(class Engine& Engine)
{
	for (auto& System : RenderSystems)
	{
		System.get().Start(Engine);
	}
}

void SystemsManager::UpdateRenderSystems(class Engine& Engine)
{
	for (auto& System : RenderSystems)
	{
		System.get().Update(Engine);
	}
}