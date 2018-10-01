#pragma once
#include "GLRenderResource.h"

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
	std::string Code;

	ShaderCompilerWorker(const std::string& Code)
		: Code(Code)
	{
	}

	template<typename T>
	void SetDefine(const std::string& Define, T&& Value)
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
		//signal_unimplemented();
	}

private:
	friend class GLShaderCompiler;
};

CLASS(GLShader);

class GLShaderCompiler
{
public:
	virtual GLShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) = 0;

	template<typename ShaderType>
	void CompileShader(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
	{
		if (!Contains(FilenameToCode, Filename))
		{
			FilenameToCode[Filename] = GPlatform->FileRead(Filename);
		}

		ShaderCompilerWorker Worker(FilenameToCode[Filename]);
		ShaderType::ModifyCompilationEnvironment(Worker);

		ShaderMetadata Meta(Filename, EntryPoint, Stage);
		GLShaderRef Shader = CompileShader(Worker, Meta);
		StoreShader(typeid(ShaderType).name(), Shader);
	}

	void StoreShader(const std::string& Type, GLShaderRef Shader);
	GLShaderRef FindShader(const std::string& Type);

protected:
	// Maps a filename to code.
	Map<std::string, std::string> FilenameToCode;
	// Maps a shader's typename to its GLShader.
	Map<std::string, GLShaderRef> Shaders;
};

CLASS(GLShaderCompiler);

extern GLShaderCompilerRef GShaderCompiler;

#define COMPILE_SHADER(Type, Filename, EntryPoint, Stage) \
	GShaderCompiler->CompileShader<Type>(Filename, EntryPoint, Stage);