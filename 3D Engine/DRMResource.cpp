#include "DRMResource.h"

namespace drm
{
	bool ImagePrivate::IsColor() const
	{
		return !(IsDepth() || IsStencil() || Format == EFormat::UNDEFINED);
	}

	bool ImagePrivate::IsStencil() const
	{
		static const std::unordered_set<EFormat> StencilFormats =
		{
			EFormat::S8_UINT
		};
		return IsDepthStencil() || StencilFormats.find(Format) != StencilFormats.end();
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

	uint32 ImagePrivate::GetSize(EFormat Format)
	{
		static HashTable<EFormat, uint32> EngineFormatStrides =
		{
			ENTRY(EFormat::R8_UNORM, 1)
			ENTRY(EFormat::R8G8B8A8_UNORM, 4)
			ENTRY(EFormat::R32_SFLOAT, 4)
			ENTRY(EFormat::R32_SINT, 4)
			ENTRY(EFormat::R32_UINT, 4)
			ENTRY(EFormat::R32G32B32A32_SFLOAT, 16)
			ENTRY(EFormat::R32G32B32_SFLOAT, 12)
			ENTRY(EFormat::R32G32_SFLOAT, 8)
		};

		return EngineFormatStrides[Format];
	}

	bool ImagePrivate::IsDepth() const
	{
		return IsDepth(Format);
	}

	uint32 ImagePrivate::GetStrideInBytes() const
	{
		return GetSize(Format);
	}
}

/** Reference: https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code */
#define POLYNOMIAL 0xD8
#define WIDTH  (8 * sizeof(Crc))
#define TOPBIT (1 << (WIDTH - 1))

static std::array<Crc, 256> GetCrcTable()
{
	std::array<Crc, 256> CrcTable;
	Crc Remainder = 0;

	for (uint32 Dividend = 0; Dividend < 256; ++Dividend)
	{
		Remainder = Dividend << (WIDTH - 8);

		for (uint32 Bit = 8; Bit > 0; --Bit)
		{
			if (Remainder & TOPBIT)
			{
				Remainder = (Remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				Remainder = (Remainder << 1);
			}
		}

		CrcTable[Dividend] = Remainder;
	}

	return CrcTable;
}

Crc CalculateCrc(const void* Message, std::size_t nBytes)
{
	static std::array<Crc, 256> CrcTable = GetCrcTable();
	const uint8* ByteMessage = static_cast<const uint8*>(Message);

	uint8 Data;
	Crc Remainder = 0;

	for (std::size_t Byte = 0; Byte < nBytes; ++Byte)
	{
		Data = ByteMessage[Byte] ^ (Remainder >> (WIDTH - 8));
		Remainder = CrcTable[Data] ^ (Remainder << 8);
	}

	return Remainder;
}