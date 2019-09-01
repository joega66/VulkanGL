#pragma once
#include <Platform/Platform.h>
#include <typeindex>

class ISystem
{
public:
	virtual void Start() {}

	virtual void Update(class Scene& Scene) {}
};

class SystemsHerder
{
public:
	SystemsHerder() = default;
	void Register(ISystem& ISystem);
	void StartSystems();
	void UpdateSystems(class Scene& Scene);

private:
	std::vector<std::reference_wrapper<ISystem>> Systems;
};

#define SYSTEM(SystemName) \
	public: \
	SystemName& operator=(const SystemName&) = delete; \
	SystemName(const SystemName&) = delete; \
	SystemName() = default; \