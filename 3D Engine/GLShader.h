#pragma once
#include "GLRenderResource.h"
#include <mutex>
#include <future>

enum class EShaderStage : uint32
{
	Vertex,
	TessControl,
	TessEvaluation,
	Geometry,
	Fragment,
	Compute
};

class ShaderCompilerWorker
{
public:
	const std::string& Code;

	ShaderCompilerWorker(const std::string& Code)
		: Code(Code)
	{
	}

	template<typename T>
	void SetDefine(const std::string& Define, const T& Value)
	{
		// @todo-joe Should just batch the defines. SPIR-V compiler has -D option for setting defines.
		signal_unimplemented();
	}
};

struct ShaderMetadata
{
	std::string Filename;
	std::string EntryPoint;
	EShaderStage Stage;

	ShaderMetadata(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
		: Filename(Filename), EntryPoint(EntryPoint), Stage(Stage)
	{
	}
};

class GLShader : public GLRenderResource
{
public:
	const ShaderMetadata Meta;

	GLShader(const ShaderMetadata& Meta)
		: Meta(Meta)
	{
	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker) 
	{
	}
};

CLASS(GLShader);

class GLShaderCompiler
{
public:
	GLShaderRef FindShader(const std::string& Type);

private:
	Map<std::string, GLShaderRef> Shaders;

	friend class ScopedAsyncShaderCompiler;
	using ShaderMapEntry = std::pair<std::string, GLShaderRef>;

	template<typename ShaderType>
	[[nodiscard]]
	ShaderMapEntry CompileShader(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
	{
		std::string Code = GPlatform->FileRead(Filename);
		ShaderCompilerWorker Worker(Code);
		ShaderType::ModifyCompilationEnvironment(Worker);
		ShaderMetadata Meta(Filename, EntryPoint, Stage);
		GLShaderRef Shader = CompileShader(Worker, Meta);
		return std::make_pair(std::string(typeid(ShaderType).name()), Shader);
	}

	virtual GLShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) const = 0;
	void StoreShader(const std::string& Type, GLShaderRef Shader);
};

CLASS(GLShaderCompiler);

extern GLShaderCompilerRef GShaderCompiler;

class ScopedAsyncShaderCompiler
{
public:
	template<typename ShaderType>
	void Compile(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
	{
		std::future<GLShaderCompiler::ShaderMapEntry> Task = std::async(std::launch::async,
			[=] () { return GShaderCompiler->CompileShader<ShaderType>(Filename, EntryPoint, Stage); });
		Tasks.push_back(std::move(Task));
	}

	~ScopedAsyncShaderCompiler()
	{
		for (auto& Task : Tasks)
		{
			auto Result = Task.get();
			GShaderCompiler->StoreShader(Result.first, Result.second);
		}
	}

private:
	std::list<std::future<GLShaderCompiler::ShaderMapEntry>> Tasks;
};