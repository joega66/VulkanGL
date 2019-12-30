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
	void SetDefine(std::string&& Define, const T& Value)
	{
		Defines.push_back(std::make_pair(std::move(Define), std::to_string(Value)));
	}

	void SetDefine(std::string&& Define)
	{
		Defines.push_back(std::make_pair(std::move(Define), "1"));
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

class SpecializationInfo
{
public:
	struct SpecializationMapEntry
	{
		uint32 ConstantID;
		uint32 Offset;
		size_t Size;

		bool operator==(const SpecializationMapEntry& Other) const
		{
			return ConstantID == Other.ConstantID
				&& Offset == Other.Offset
				&& Size == Other.Size;
		}
	};

	SpecializationInfo() = default;
		
	template<typename SpecializationConstantType>
	void Add(uint32 ConstantID, const SpecializationConstantType& Constant)
	{
		const uint32 Offset = Data.size();
		Data.resize(Offset + sizeof(Constant));
		memcpy(Data.data() + Offset, &Constant, sizeof(Constant));
		const SpecializationMapEntry MapEntry = { ConstantID, Offset, sizeof(Constant) };
		MapEntries.push_back(std::move(MapEntry));
	}

	bool operator==(const SpecializationInfo& Other) const
	{
		return MapEntries == Other.MapEntries
			&& Data == Other.Data;
	}

	inline const std::vector<SpecializationMapEntry>& GetMapEntries() const { return MapEntries; }

	inline const std::vector<uint8>& GetData() const { return Data; }

private:
	std::vector<SpecializationMapEntry> MapEntries;
	std::vector<uint8> Data;
};

class ShaderCompilationInfo
{
public:
	std::type_index Type;
	EShaderStage Stage;
	std::string Entrypoint;
	std::string Filename;
	uint64 LastWriteTime;
	ShaderCompilerWorker Worker;

	ShaderCompilationInfo(
		std::type_index Type,
		EShaderStage Stage, 
		const std::string& Entrypoint,
		const std::string& Filename,
		uint64 LastWriteTime,
		const ShaderCompilerWorker& Worker)
		: Type(Type)
		, Stage(Stage)
		, Entrypoint(Entrypoint)
		, Filename(Filename)
		, LastWriteTime(LastWriteTime)
		, Worker(Worker)
	{
	}
};

namespace drm
{
	class Shader
	{
	public:
		ShaderCompilationInfo CompilationInfo;

		Shader(const ShaderCompilationInfo& CompilationInfo)
			: CompilationInfo(CompilationInfo)
		{
		}

		static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
		{
		}
	};

	CLASS(Shader);
}