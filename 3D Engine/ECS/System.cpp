#include "System.h"

void SystemsHerder::Register(ISystem& System)
{
	Systems.push_back(System);
}

void SystemsHerder::StartSystems()
{
	for (auto& System : Systems)
	{
		System.get().Start();
	}
}

void SystemsHerder::UpdateSystems(Scene& Scene)
{
	for (auto& System : Systems)
	{
		System.get().Update(Scene);
	}
}