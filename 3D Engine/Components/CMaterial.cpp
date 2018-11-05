#include "CMaterial.h"

CMaterial::CMaterial(GLImageRef Material, EMaterialType MaterialType)
	: Material(Material), MaterialType(MaterialType)
{
}

EMaterialType CMaterial::GetMaterialType() const
{ 
	return MaterialType; 
}

GLImageRef CMaterial::GetMaterial() const
{
	return Material;
}

MaterialProxy::MaterialProxy(const std::list<CMaterialRef>& Materials)
	: Materials(Materials)
{
}

void MaterialProxy::Add(CMaterialRef Material)
{
	Materials.push_back(Material);
}

uint32 MaterialProxy::Count(EMaterialType MaterialType) const
{
	return std::count_if(Materials.begin(), Materials.end(), [&] (const CMaterialRef& Material)
	{
		return Material->GetMaterialType() == MaterialType;
	});
}

bool MaterialProxy::IsEmpty() const
{
	return Materials.empty();
}

CMaterialRef MaterialProxy::Get(EMaterialType MaterialType) const
{
	if (auto Iter = std::find_if(Materials.begin(), Materials.end(), [&] (const CMaterialRef& Material)
	{
		return Material->GetMaterialType() == MaterialType;
	}); Iter != Materials.end())
	{
		return *Iter;
	}
	else
	{
		return nullptr;
	}
}

void MaterialProxy::Merge(std::shared_ptr<MaterialProxy> Materials)
{
	auto& MaterialList = Materials->Materials;
	std::for_each(MaterialList.begin(), MaterialList.end(), [&] (CMaterialRef Material)
	{
		Add(Material);
	});
}