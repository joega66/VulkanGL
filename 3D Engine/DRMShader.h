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

struct ShaderBinding
{
	uint32 Location = -1;

	operator uint32() const
	{
		return Location;
	}

	operator bool() const
	{
		return Location != -1;
	}
};

class ShaderResourceTable
{
public:
	const std::type_index Type;
	const EShaderStage Stage;
	const std::string Entrypoint;

	ShaderResourceTable(std::type_index Type, EShaderStage Stage, const std::string& Entrypoint, const HashTable<std::string, uint32>& UniformLocations)
		: Type(Type), Stage(Stage), Entrypoint(Entrypoint), UniformLocations(UniformLocations)
	{
	}

	void Bind(const std::string& Name, ShaderBinding& Binding) const
	{
		if (Contains(UniformLocations, Name))
		{
			Binding.Location = GetValue(UniformLocations, Name);
		}
		else
		{
			Binding.Location = -1;
		}
	}

private:
	HashTable<std::string, uint32> UniformLocations;
};

namespace drm
{
	class Shader : public std::enable_shared_from_this<Shader>
	{
	public:
		const std::type_index Type;
		const EShaderStage Stage;
		const std::string Entrypoint;

		Shader(const ShaderResourceTable& ResourceTable)
			: Type(ResourceTable.Type), Stage(ResourceTable.Stage), Entrypoint(ResourceTable.Entrypoint)
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