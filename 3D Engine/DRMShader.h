#pragma once
#include <DRMResource.h>
#include <typeindex>

/** Serializes C++ types to shader types. */
class ShaderTypeSerializer
{
public:
	template<typename T>
	static std::string Serialize();
};

struct ShaderStructDecl
{
	std::string decl;
	uint32 size;
};

#define BEGIN_SHADER_STRUCT(StructName)	\
struct StructName						\
{										\
private:								\
	struct FirstMemberId {};			\
	typedef FirstMemberId				\

#define SHADER_PARAMETER(ShaderType, Name)																		\
		MemberId##Name;																							\
public:																											\
	ShaderType Name;																							\
private:																										\
	static void Serialize(MemberId##Name MemberId, std::string& ShaderStruct)									\
	{																											\
		ShaderStruct += ShaderTypeSerializer::Serialize<ShaderType>() + " " + std::string(#Name) + ";";			\
		Serialize(NextMemberId##Name{}, ShaderStruct);															\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

#define END_SHADER_STRUCT(StructName)																			\
		LastMemberId;																							\
	static void Serialize(LastMemberId MemberId, std::string& ShaderStruct) {}									\
public:																											\
	static std::string Serialize()																				\
	{																											\
		std::string ShaderStruct;																				\
		Serialize(FirstMemberId{}, ShaderStruct);																\
		return ShaderStruct;																					\
	}																											\
	struct Decl##StructName : ShaderStructDecl{																	\
		Decl##StructName() {																					\
			decl = Serialize();																				\
			size = sizeof(StructName);																			\
		}																										\
	};																											\
	static Decl##StructName Decl;																				\
};																												\
StructName::Decl##StructName StructName::Decl;																	\

class ShaderCompilerWorker
{
public:
	using ShaderDefines = std::vector<std::pair<std::string, std::string>>;

	ShaderCompilerWorker() = default;

	/** Set a shader define (templated version). */
	template<typename T>
	void SetDefine(std::string&& define, const T& value)
	{
		_Defines.push_back(std::make_pair(std::move(define), std::to_string(value)));
	}

	/** Set a shader define (stringified version). */
	void SetDefine(std::string&& define)
	{
		_Defines.push_back(std::make_pair(std::move(define), "1"));
	}

	/** Get all shader defines. */
	const ShaderDefines& GetDefines() const
	{
		return _Defines;
	}

	/** Define the shader's push constant struct. */
	inline void operator<<(const ShaderStructDecl& shaderStructDecl)
	{
		_PushConstantStruct = shaderStructDecl.decl;
		_PushConstantSize = shaderStructDecl.size;
	}

	/** Get the shader's push constant struct. */
	inline uint32 GetPushConstantOffset() const { return _PushConstantOffset; }
	inline uint32 GetPushConstantSize() const { return _PushConstantSize; }
	inline const std::string& GetPushConstantStruct() const { return _PushConstantStruct; }

private:
	ShaderDefines _Defines;
	uint32 _PushConstantOffset = 0;
	uint32 _PushConstantSize = 0;
	std::string _PushConstantStruct = "";
};

struct ShaderInfo
{
	std::string filename;
	std::string entrypoint;
	EShaderStage stage;
};

class ShaderCompilationInfo
{
public:
	std::type_index type;
	EShaderStage stage;
	std::string entrypoint;
	std::string filename;
	uint64 lastWriteTime;
	ShaderCompilerWorker worker;
	void* module;
	std::vector<VertexAttributeDescription> vertexAttributeDescriptions;
	PushConstantRange pushConstantRange;

	ShaderCompilationInfo(
		std::type_index type,
		EShaderStage stage, 
		const std::string& entrypoint,
		const std::string& filename,
		uint64 lastWriteTime,
		const ShaderCompilerWorker& worker,
		void* module,
		const std::vector<VertexAttributeDescription>& vertexAttributeDescriptions,
		const PushConstantRange& pushConstantRange)
		: type(type)
		, stage(stage)
		, entrypoint(entrypoint)
		, filename(filename)
		, lastWriteTime(lastWriteTime)
		, worker(worker)
		, module(module)
		, vertexAttributeDescriptions(vertexAttributeDescriptions)
		, pushConstantRange(pushConstantRange)
	{
	}
};

namespace drm
{
	class Shader
	{
	public:
		ShaderCompilationInfo compilationInfo;

		Shader(const ShaderCompilationInfo& compilationInfo)
			: compilationInfo(compilationInfo)
		{
		}

		static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
		{
		}
	};

	/** The shader library compiles shaders and caches them by typename. */
	class ShaderLibrary
	{
	public:
		/** Find shader of ShaderType. */
		template<typename ShaderType>
		const ShaderType* FindShader()
		{
			std::type_index type = std::type_index(typeid(ShaderType));

			if (_Shaders.contains(type))
			{
				return static_cast<const ShaderType*>(_Shaders[type].get());
			}
			else
			{
				ShaderCompilerWorker worker;
				ShaderType::SetEnvironmentVariables(worker);
				const auto& [filename, entrypoint, stage] = ShaderType::GetShaderInfo();
				const ShaderCompilationInfo compilationInfo = CompileShader(worker, filename, entrypoint, stage, type);
				_Shaders.emplace(compilationInfo.type, std::make_unique<ShaderType>(compilationInfo));
				return static_cast<const ShaderType*>(_Shaders[compilationInfo.type].get());
			}
		}

		/** Recompile cached shaders. */
		virtual void RecompileShaders() = 0;

	private:
		/** Compile the shader. */
		virtual ShaderCompilationInfo CompileShader(
			const ShaderCompilerWorker& worker,
			const std::string& filename,
			const std::string& entryPoint,
			EShaderStage stage,
			std::type_index type
		) = 0;

	protected:
		/** Cached shaders. */
		std::unordered_map<std::type_index, std::unique_ptr<drm::Shader>> _Shaders;

	};
}