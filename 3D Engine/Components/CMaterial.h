#pragma once
#include "Component.h"
#include "../GLRenderResource.h"

enum class EMaterialType
{
	Diffuse,
	Normal,
};

class CMaterial : public Component<CMaterial>
{
public:
	CMaterial(GLImageRef Material, EMaterialType MaterialType);

	EMaterialType GetMaterialType() const;
	GLImageRef GetMaterial() const;

private:
	EMaterialType MaterialType;
	GLImageRef Material;
};

CLASS(CMaterial);

class MaterialProxy
{
public:
	MaterialProxy() = default;
	MaterialProxy(const std::list<CMaterialRef>& Materials);

	void Add(CMaterialRef Material);
	uint32 Count(EMaterialType MaterialType) const;
	bool IsEmpty() const;
	CMaterialRef Get(EMaterialType MaterialType) const;
	void Merge(std::shared_ptr<MaterialProxy> Materials);

private:
	std::list<CMaterialRef> Materials;
};

CLASS(MaterialProxy);