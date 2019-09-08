#pragma once
#include <Platform/Platform.h>
#include <typeindex>

class ISystem
{
public:
	virtual void Start(class Scene& Scene) {}
	virtual void Update(class Scene& Scene) {}
};

class IRenderSystem
{
public:
	virtual void Update(class SceneProxy& Scene) {}
};

class SystemsManager
{
public:
	SystemsManager() = default;
	void Register(ISystem& ISystem);
	void StartSystems(class Scene& Scene);
	void UpdateSystems(class Scene& Scene);
	void UpdateRenderSystems(class SceneProxy& Scene);

private:
	std::vector<std::reference_wrapper<ISystem>> Systems;
	std::vector<std::reference_wrapper<IRenderSystem>> RenderSystems;
};

#define SYSTEM(SystemName) \
	public: \
	SystemName& operator=(const SystemName&) = delete; \
	SystemName(const SystemName&) = delete; \
	SystemName() = default; \