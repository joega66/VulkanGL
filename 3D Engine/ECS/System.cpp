#include "System.h"

void SystemsManager::Register(ISystem& system)
{
	_Systems.push_back(system);
}

void SystemsManager::StartSystems(Engine& engine)
{
	for (auto& system : _Systems)
	{
		system.get().Start(engine);
	}
}

void SystemsManager::UpdateSystems(Engine& engine)
{
	for (auto& system : _Systems)
	{
		system.get().Update(engine);
	}
}