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
	virtual void Start(class EntityManager& ECS, class DRM& Device) {}
	virtual void Update(class EntityManager& ECS, class DRM& Device) {}
};

class SystemsManager
{
public:
	SystemsManager() = default;
	void Register(ISystem& ISystem);
	void Register(IRenderSystem& System);
	void StartSystems(class Scene& Scene);
	void UpdateSystems(class Scene& Scene);
	void StartRenderSystems(class EntityManager& ECS, class DRM& Device);
	void UpdateRenderSystems(class EntityManager& ECS, class DRM& Device);

private:
	std::vector<std::reference_wrapper<ISystem>> Systems;
	std::vector<std::reference_wrapper<IRenderSystem>> RenderSystems;
};

#define SYSTEM(SystemName) \
	public: \
	SystemName& operator=(const SystemName&) = delete; \
	SystemName(const SystemName&) = delete; \
	SystemName() = default; \