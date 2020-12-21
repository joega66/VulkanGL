#pragma once
#include <Platform/Platform.h>
#include <typeindex>

class Engine;

class ISystem
{
public:
	virtual void Start(Engine& engine) {}
	virtual void Update(Engine& engine) {}
};

class SystemsManager
{
public:
	SystemsManager() = default;
	void Register(ISystem& system);
	void StartSystems(Engine& engine);
	void UpdateSystems(Engine& engine);

private:
	std::vector<std::reference_wrapper<ISystem>> _Systems;
};