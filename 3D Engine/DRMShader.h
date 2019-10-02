#pragma once
#include <Platform/Platform.h>
#include <typeindex>

enum class EShaderStage
{
	Vertex = 1 << 0,
	TessControl = 1 << 1,
	TessEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
	All = Vertex | TessControl | TessEvaluation | Geometry | Fragment | Compute
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
	EShaderStage StageFlags = EShaderStage::All;
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
		const HashTable<std::string, ShaderBinding>& Bindings)
		: Type(Type), Stage(Stage), Entrypoint(Entrypoint), Bindings(Bindings)
	{
	}

	void Bind(const std::string& Name, ShaderBinding& Binding) const
	{
		if (auto Iter = Bindings.find(Name); Iter != Bindings.end())
		{
			Binding = Iter->second;
		}
	}

private:
	HashTable<std::string, ShaderBinding> Bindings;
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