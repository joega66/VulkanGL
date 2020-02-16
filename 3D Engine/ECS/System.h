#pragma once
#include <Platform/Platform.h>
#include <typeindex>

class ISystem
{
public:
	virtual void Start(class Engine& Engine) {}
	virtual void Update(class Engine& Engine) {}
};

class IRenderSystem
{
public:
	virtual void Start(class Engine& Engine) {}
	virtual void Update(class Engine& Engine) {}
};

class SystemsManager
{
public:
	SystemsManager() = default;
	void Register(ISystem& ISystem);
	void Register(IRenderSystem& System);
	void StartSystems(class Engine& Engine);
	void UpdateSystems(class Engine& Engine);
	void StartRenderSystems(class Engine& Engine);
	void UpdateRenderSystems(class Engine& Engine);

private:
	std::vector<std::reference_wrapper<ISystem>> Systems;
	std::vector<std::reference_wrapper<IRenderSystem>> RenderSystems;
};

#define SYSTEM(SystemName) \
	public: \
	SystemName& operator=(const SystemName&) = delete; \
	SystemName(const SystemName&) = delete; \
	SystemName() = default; \