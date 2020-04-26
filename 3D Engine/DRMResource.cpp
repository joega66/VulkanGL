#include "DRMResource.h"
#include <unordered_set>

namespace drm
{
	bool ImagePrivate::IsColor(EFormat Format)
	{
		return !(IsDepth(Format) || IsStencil(Format) || Format == EFormat::UNDEFINED);
	}

	bool ImagePrivate::IsColor() const
	{
		return IsColor(Format);
	}

	bool ImagePrivate::IsStencil(EFormat Format)
	{
		static const std::unordered_set<EFormat> StencilFormats =
		{
			EFormat::S8_UINT
		};
		return IsDepthStencil(Format) || StencilFormats.find(Format) != StencilFormats.end();
	}

	bool ImagePrivate::IsStencil() const
	{
		return IsStencil(Format);
	}

	bool ImagePrivate::IsDepthStencil(EFormat Format)
	{
		static const std::unordered_set<EFormat> DepthFormats =
		{
			EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool ImagePrivate::IsDepthStencil() const
	{
		return IsDepthStencil(Format);
	}

	bool ImagePrivate::IsDepth(EFormat Format)
	{
		static const std::unordered_set<EFormat> DepthFormats =
		{
			EFormat::D16_UNORM, EFormat::D32_SFLOAT, EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return DepthFormats.find(Format) != DepthFormats.end();
	}

	bool ImagePrivate::IsDepth() const
	{
		return IsDepth(Format);
	}

	uint32 ImagePrivate::GetSize(EFormat Format)
	{
		static std::unordered_map<EFormat, uint32> EngineFormatStrides =
		{
			ENTRY(EFormat::R8_UNORM, 1)
			ENTRY(EFormat::R8G8B8A8_UNORM, 4)
			ENTRY(EFormat::R32_SFLOAT, 4)
			ENTRY(EFormat::R32_SINT, 4)
			ENTRY(EFormat::R32_UINT, 4)
			ENTRY(EFormat::R16G16B16A16_SFLOAT, 8) 
			ENTRY(EFormat::R32G32B32A32_SFLOAT, 16)
			ENTRY(EFormat::R32G32B32_SFLOAT, 12)
			ENTRY(EFormat::R32G32_SFLOAT, 8)
		};

		return EngineFormatStrides[Format];
	}

	uint32 ImagePrivate::GetStrideInBytes() const
	{
		return GetSize(Format);
	}
}