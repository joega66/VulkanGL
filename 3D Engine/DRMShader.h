#pragma once
#include <DRMResource.h>
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

struct ShaderInfo
{
	std::string Filename;
	std::string Entrypoint;
	EShaderStage Stage;
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

struct VertexAttributeDescription
{
	uint64	Location;
	uint32	Binding;
	EFormat	Format;
	uint32	Offset;

	friend bool operator==(const VertexAttributeDescription& L, const VertexAttributeDescription& R)
	{
		return L.Location == R.Location
			&& L.Binding == R.Binding
			&& L.Format == R.Format
			&& L.Offset == R.Offset;
	}
};

struct VertexBindingDescription
{
	uint32 Binding;
	uint32 Stride;

	friend bool operator==(const VertexBindingDescription& L, const VertexBindingDescription& R)
	{
		return L.Binding == R.Binding
			&& L.Stride == R.Stride;
	}
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
	uint64 Module;
	std::vector<VertexAttributeDescription> VertexAttributeDescriptions;

	ShaderCompilationInfo(
		std::type_index Type,
		EShaderStage Stage, 
		const std::string& Entrypoint,
		const std::string& Filename,
		uint64 LastWriteTime,
		const ShaderCompilerWorker& Worker,
		uint64 Module,
		const std::vector<VertexAttributeDescription>& VertexAttributeDescriptions)
		: Type(Type)
		, Stage(Stage)
		, Entrypoint(Entrypoint)
		, Filename(Filename)
		, LastWriteTime(LastWriteTime)
		, Worker(Worker)
		, Module(Module)
		, VertexAttributeDescriptions(VertexAttributeDescriptions)
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

/** The Shader Map compiles shaders and caches them by typename. */
class DRMShaderMap
{
public:
	/** Find shader of ShaderType. */
	template<typename ShaderType>
	std::shared_ptr<ShaderType> FindShader()
	{
		std::type_index Type = std::type_index(typeid(ShaderType));

		if (Contains(Shaders, Type))
		{
			return std::static_pointer_cast<ShaderType>(Shaders[Type]);
		}
		else
		{
			ShaderCompilerWorker Worker;
			ShaderType::SetEnvironmentVariables(Worker);
			const auto& [Filename, EntryPoint, Stage] = ShaderType::GetShaderInfo();
			const ShaderCompilationInfo CompilationInfo = CompileShader(Worker, Filename, EntryPoint, Stage, Type);
			std::shared_ptr<ShaderType> Shader = MakeRef<ShaderType>(CompilationInfo);
			Shaders[Shader->CompilationInfo.Type] = Shader;
			return Shader;
		}
	}

	/** Recompile cached shaders. */
	virtual void RecompileShaders() = 0;

private:
	/** Compile the shader. */
	virtual ShaderCompilationInfo CompileShader(
		const ShaderCompilerWorker& Worker,
		const std::string& Filename,
		const std::string& EntryPoint,
		EShaderStage Stage,
		std::type_index Type
	) = 0;

protected:
	/** Cached shaders. */
	HashTable<std::type_index, drm::ShaderRef> Shaders;

};