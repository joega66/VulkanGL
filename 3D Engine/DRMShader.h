#pragma once
#include <DRMResource.h>
#include <typeindex>

enum class EShaderStage
{
	None = 0,
	Vertex = 1 << 0,
	TessControl = 1 << 1,
	TessEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
	AllGraphics = Vertex | TessControl | TessEvaluation | Geometry | Fragment,
	All = AllGraphics | Compute
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

class ShaderCompilationInfo
{
public:
	std::type_index Type;
	EShaderStage Stage;
	std::string Entrypoint;
	std::string Filename;
	uint64 LastWriteTime;
	ShaderCompilerWorker Worker;
	void* Module;
	std::vector<VertexAttributeDescription> VertexAttributeDescriptions;

	ShaderCompilationInfo(
		std::type_index Type,
		EShaderStage Stage, 
		const std::string& Entrypoint,
		const std::string& Filename,
		uint64 LastWriteTime,
		const ShaderCompilerWorker& Worker,
		void* Module,
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
}

/** The Shader Map compiles shaders and caches them by typename. */
class DRMShaderMap
{
public:
	/** Find shader of ShaderType. */
	template<typename ShaderType>
	const ShaderType* FindShader()
	{
		std::type_index Type = std::type_index(typeid(ShaderType));

		if (Contains(Shaders, Type))
		{
			return static_cast<const ShaderType*>(Shaders[Type].get());
		}
		else
		{
			ShaderCompilerWorker Worker;
			ShaderType::SetEnvironmentVariables(Worker);
			const auto& [Filename, EntryPoint, Stage] = ShaderType::GetShaderInfo();
			const ShaderCompilationInfo CompilationInfo = CompileShader(Worker, Filename, EntryPoint, Stage, Type);
			Shaders.emplace(CompilationInfo.Type, std::make_unique<ShaderType>(CompilationInfo));
			return static_cast<const ShaderType*>(Shaders[CompilationInfo.Type].get());
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
	HashTable<std::type_index, std::unique_ptr<drm::Shader>> Shaders;

};