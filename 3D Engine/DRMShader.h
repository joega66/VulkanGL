#pragma once
#include <Platform/Platform.h>
#include <typeindex>

enum class EShaderStage
{
	Vertex,
	TessControl,
	TessEvaluation,
	Geometry,
	Fragment,
	Compute,
	//All = Vertex | TessControl | TessEvaluation | Geometry | Fragment | Compute
};

enum
{
	NumGraphicsStages = (int32)EShaderStage::Fragment + 1
};

class ShaderCompilerWorker
{
public:
	using ShaderDefines = std::vector<std::pair<std::string, std::string>>;

	ShaderCompilerWorker() = default;

	template<typename T>
	void SetDefine(const std::string& Define, const T& Value)
	{
		Defines.push_back(std::make_pair(Define, std::to_string(Value)));
	}

	void SetDefine(const std::string& Define)
	{
		Defines.push_back(std::make_pair(Define, "1"));
	}

	const ShaderDefines& GetDefines() const
	{
		return Defines;
	}

private:
	ShaderDefines Defines;
};

struct ShaderMetadata
{
	std::string Filename;
	std::string EntryPoint;
	EShaderStage Stage;
	std::type_index Type;

	ShaderMetadata(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage, std::type_index Type)
		: Filename(Filename), EntryPoint(EntryPoint), Stage(Stage), Type(Type)
	{
	}
};

struct ShaderInfo
{
	std::string Filename;
	std::string Entrypoint;
	EShaderStage Stage;
};

class ShaderBinding
{
public:
	ShaderBinding() = default;

	ShaderBinding(uint32 Binding)
		: Binding(Binding)
	{
	}

	inline uint32 GetBinding() const { return Binding; }

	operator uint32() const
	{
		return Binding;
	}

	operator bool() const
	{
		return Binding != -1;
	}

private:
	uint32 Binding = -1;
	//EShaderStage StageFlags = EShaderStage::All;
};

class SpecConstant
{
public:
	SpecConstant() = default;

	SpecConstant(uint32 ConstantID)
		: ConstantID(ConstantID)
	{
	}

	operator uint32() const
	{
		return ConstantID;
	}

private:
	uint32 ConstantID = -1;
};

class SpecializationInfo
{
public:
	struct SpecMapEntry
	{
		uint32 ConstantID;
		uint32 Offset;
		size_t Size;

		bool operator==(const SpecMapEntry& Other) const
		{
			return ConstantID == Other.ConstantID
				&& Offset == Other.Offset
				&& Size == Other.Size;
		}
	};

	SpecializationInfo() = default;
		
	template<typename SpecConstantType>
	void Add(const SpecConstant& SpecConstant, const SpecConstantType& Constant)
	{
		const uint32 Offset = Data.size();
		Data.resize(Offset + sizeof(Constant));
		memcpy(Data.data() + Offset, &Constant, sizeof(Constant));
		const SpecMapEntry Entry = { SpecConstant, Offset, sizeof(Constant) };
		Entries.push_back(std::move(Entry));
	}

	bool operator==(const SpecializationInfo& Other) const
	{
		return Entries == Other.Entries
			&& Data == Other.Data;
	}

	const std::vector<SpecMapEntry>& GetEntries() const { return Entries; }

	const std::vector<uint8>& GetData() const { return Data; }

private:
	std::vector<SpecMapEntry> Entries;
	std::vector<uint8> Data;
};

class ShaderResourceTable
{
public:
	const std::type_index Type;
	const EShaderStage Stage;
	const std::string Entrypoint;

	ShaderResourceTable(
		std::type_index Type, 
		EShaderStage Stage, 
		const std::string& Entrypoint, 
		const HashTable<std::string, ShaderBinding>& Bindings,
		const HashTable<std::string, SpecConstant>& SpecConstants)
		: Type(Type), Stage(Stage), Entrypoint(Entrypoint), Bindings(Bindings), SpecConstants(SpecConstants)
	{
	}

	void Bind(const std::string& Name, ShaderBinding& Binding) const
	{
		if (auto Iter = Bindings.find(Name); Iter != Bindings.end())
		{
			Binding = Iter->second;
		}
		else
		{
			LOG("Shader %s does not have binding %s", Type.name(), Name.c_str());
		}
	}

	void Bind(const std::string& Name, SpecConstant& SpecConstant) const
	{
		if (auto Iter = SpecConstants.find(Name); Iter != SpecConstants.end())
		{
			SpecConstant = Iter->second;
		}
		else
		{
			LOG("Shader %s does not have spec constant %s", Type.name(), Name.c_str());
		}
	}

private:
	HashTable<std::string, ShaderBinding> Bindings;
	HashTable<std::string, SpecConstant> SpecConstants;
};

namespace drm
{
	class Shader : public std::enable_shared_from_this<Shader>
	{
	public:
		const ShaderResourceTable ResourceTable;

		Shader(const ShaderResourceTable& ResourceTable)
			: ResourceTable(ResourceTable)
		{
		}

		static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
		{
		}

		std::shared_ptr<Shader> GetShader()
		{
			return shared_from_this();
		}
	};

	CLASS(Shader);
}