#pragma once
#include "DRMResource.h"
#include <typeindex>

enum class EShaderStage
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

class GLShader
{
public:
	const ShaderMetadata Meta;

	GLShader(const ShaderMetadata& Meta, const HashTable<std::string, uint32>& AttributeLocations, const HashTable<std::string, uint32>& UniformLocations)
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
	HashTable<std::string, uint32> AttributeLocations;
	HashTable<std::string, uint32> UniformLocations;
};

CLASS(GLShader);

class GLShaderCompiler
{
public:
	GLShaderRef FindShader(std::type_index Type);

	template<typename ShaderType>
	GLShaderRef CompileShader(const std::string& Filename, const std::string& EntryPoint, EShaderStage Stage)
	{
		ShaderCompilerWorker Worker;
		ShaderType::ModifyCompilationEnvironment(Worker);
		ShaderMetadata Meta(Filename, EntryPoint, Stage);
		GLShaderRef Shader = CompileShader(Worker, Meta);
		StoreShader(std::type_index(typeid(ShaderType)), Shader);
		return Shader;
	}

private:
	virtual GLShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) const = 0;
	void StoreShader(std::type_index Type, GLShaderRef Shader);

	HashTable<std::type_index, GLShaderRef> Shaders;
};

CLASS(GLShaderCompiler);

extern GLShaderCompilerRef GShaderCompiler;

struct GraphicsPipeline
{
	GLShaderRef Vertex;
	GLShaderRef TessControl;
	GLShaderRef TessEval;
	GLShaderRef Geometry;
	GLShaderRef Fragment;

	GraphicsPipeline() = default;
	GraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
		: Vertex(Vertex), TessControl(TessControl), TessEval(TessEval), Geometry(Geometry), Fragment(Fragment)
	{
	}
};