#pragma once
#include "GLRenderResource.h"

enum class EShaderStage : int32
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
	using ShaderDefines = std::list<std::pair<std::string, std::string>>;

	ShaderCompilerWorker() = default;

	template<typename T>
	void SetDefine(const std::string& Define, const T& Value)
	{
		Defines.push_back(std::make_pair(Define, std::string(Value)));
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

	ShaderMetadata(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
		: Filename(Filename), EntryPoint(EntryPoint), Stage(Stage)
	{
	}
};

using GLBaseShaderInfo = std::tuple<std::string, std::string, EShaderStage>;

class GLShader : public GLRenderResource
{
public:
	const ShaderMetadata Meta;

	GLShader(const ShaderMetadata& Meta, const Map<std::string, uint32>& AttributeLocations, const Map<std::string, uint32>& UniformLocations)
		: Meta(Meta), AttributeLocations(AttributeLocations), UniformLocations(UniformLocations)
	{
	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker) 
	{
	}

	uint32 GetAttributeLocation(const std::string& Name) const
	{
		return GetValue(AttributeLocations, Name);
	}

	uint32 GetUniformLocation(const std::string& Name) const
	{
		return GetValue(UniformLocations, Name);
	}

private:
	Map<std::string, uint32> UniformLocations;
	Map<std::string, uint32> AttributeLocations;
};

CLASS(GLShader);

class GLShaderCompiler
{
public:
	GLShaderRef FindShader(const std::string& Type);

	template<typename ShaderType>
	GLShaderRef CompileShader(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
	{
		ShaderCompilerWorker Worker;
		ShaderType::ModifyCompilationEnvironment(Worker);
		ShaderMetadata Meta(Filename, EntryPoint, Stage);
		GLShaderRef Shader = CompileShader(Worker, Meta);
		StoreShader(std::string(typeid(ShaderType).name()), Shader);
		return Shader;
	}

private:
	virtual GLShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) const = 0;

	void StoreShader(const std::string& Type, GLShaderRef Shader);

	Map<std::string, GLShaderRef> Shaders;
};

CLASS(GLShaderCompiler);

extern GLShaderCompilerRef GShaderCompiler;