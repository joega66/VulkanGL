#include "DRMResource.h"
#include <unordered_set>

namespace drm
{
	bool ImagePrivate::IsColor(EFormat format)
	{
		return !(IsDepth(format) || IsStencil(format) || format == EFormat::UNDEFINED);
	}

	bool ImagePrivate::IsColor() const
	{
		return IsColor(_Format);
	}

	bool ImagePrivate::IsStencil(EFormat format)
	{
		static const std::unordered_set<EFormat> stencilFormats =
		{
			EFormat::S8_UINT
		};
		return IsDepthStencil(format) || stencilFormats.find(format) != stencilFormats.end();
	}

	bool ImagePrivate::IsStencil() const
	{
		return IsStencil(_Format);
	}

	bool ImagePrivate::IsDepthStencil(EFormat format)
	{
		static const std::unordered_set<EFormat> depthFormats =
		{
			EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return depthFormats.find(format) != depthFormats.end();
	}

	bool ImagePrivate::IsDepthStencil() const
	{
		return IsDepthStencil(_Format);
	}

	bool ImagePrivate::IsDepth(EFormat format)
	{
		static const std::unordered_set<EFormat> depthFormats =
		{
			EFormat::D16_UNORM, EFormat::D32_SFLOAT, EFormat::D32_SFLOAT_S8_UINT, EFormat::D24_UNORM_S8_UINT
		};
		return depthFormats.find(format) != depthFormats.end();
	}

	bool ImagePrivate::IsDepth() const
	{
		return IsDepth(_Format);
	}

	uint32 ImagePrivate::GetSize(EFormat format)
	{
		static std::unordered_map<EFormat, uint32> engineFormatStrides =
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

		return engineFormatStrides[format];
	}

	uint32 ImagePrivate::GetStrideInBytes() const
	{
		return GetSize(_Format);
	}
}

bool SpecializationInfo::SpecializationMapEntry::operator==(const SpecializationMapEntry& other) const
{
	return constantID == other.constantID
		&& offset == other.offset
		&& size == other.size;
}

bool SpecializationInfo::operator==(const SpecializationInfo& other) const
{
	return _MapEntries == other._MapEntries
		&& _Data == other._Data;
}