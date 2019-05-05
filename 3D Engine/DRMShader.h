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

using BaseShaderInfo = std::tuple<std::string, std::string, EShaderStage>;

namespace drm
{
	class Shader
	{
	public:
		const ShaderMetadata Meta;

		Shader(const ShaderMetadata& Meta, const HashTable<std::string, uint32>& AttributeLocations, const HashTable<std::string, uint32>& UniformLocations)
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

	CLASS(Shader);
}