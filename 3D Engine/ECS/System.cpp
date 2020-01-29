#include "System.h"

void SystemsManager::Register(ISystem& System)
{
	Systems.push_back(System);
}

void SystemsManager::Register(IRenderSystem& System)
{
	RenderSystems.push_back(System);
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

void SystemsManager::StartRenderSystems(EntityManager& ECS, DRMDevice& Device)
{
	for (auto& System : RenderSystems)
	{
		System.get().Start(ECS, Device);
	}
}

void SystemsManager::UpdateRenderSystems(EntityManager& ECS, DRMDevice& Device)
{
	for (auto& System : RenderSystems)
	{
		System.get().Update(ECS, Device);
	}
}