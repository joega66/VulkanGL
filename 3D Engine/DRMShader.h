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

#define END_SHADER_STRUCT()																						\
		LastMemberId;																							\
	static void Serialize(LastMemberId MemberId, std::string& ShaderStruct) {}									\
public:																											\
	static std::string Serialize()																				\
	{																											\
		std::string ShaderStruct;																				\
		Serialize(FirstMemberId{}, ShaderStruct);																\
		return ShaderStruct;																					\
	}																											\
};																												\

class ShaderCompilerWorker
{
public:
	using ShaderDefines = std::vector<std::pair<std::string, std::string>>;

	ShaderCompilerWorker() = default;

	/** Set a shader define (templated version). */
	template<typename T>
	void SetDefine(std::string&& Define, const T& Value)
	{
		Defines.push_back(std::make_pair(std::move(Define), std::to_string(Value)));
	}

	/** Set a shader define (stringified version). */
	void SetDefine(std::string&& Define)
	{
		Defines.push_back(std::make_pair(std::move(Define), "1"));
	}

	/** Get all shader defines. */
	const ShaderDefines& GetDefines() const
	{
		return Defines;
	}

	/** Define the shader's push constant struct. */
	template<typename PushConstantType>
	inline void SetPushConstantRange(uint32 Offset = 0)
	{
		PushConstantOffset = Offset;
		PushConstantSize = sizeof(PushConstantType);
		PushConstantStruct = PushConstantType::Serialize();
	}

	/** Get the shader's push constant struct. */
	inline uint32 GetPushConstantOffset() const { return PushConstantOffset; }
	inline uint32 GetPushConstantSize() const { return PushConstantSize; }
	inline const std::string& GetPushConstantStruct() const { return PushConstantStruct; }

private:
	ShaderDefines Defines;
	uint32 PushConstantOffset = 0;
	uint32 PushConstantSize = 0;
	std::string PushConstantStruct = "";
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
		const std::vector<VertexAttributeDescription>& VertexAttributeDescriptions,
		const PushConstantRange& PushConstantRange)
		: Type(Type)
		, Stage(Stage)
		, Entrypoint(Entrypoint)
		, Filename(Filename)
		, LastWriteTime(LastWriteTime)
		, Worker(Worker)
		, Module(Module)
		, VertexAttributeDescriptions(VertexAttributeDescriptions)
		, PushConstantRange(PushConstantRange)
	{
	}

	PushConstantRange PushConstantRange;
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

	/** The shader library compiles shaders and caches them by typename. */
	class ShaderLibrary
	{
	public:
		/** Find shader of ShaderType. */
		template<typename ShaderType>
		const ShaderType* FindShader()
		{
			std::type_index Type = std::type_index(typeid(ShaderType));

			if (Shaders.contains(Type))
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
		std::unordered_map<std::type_index, std::unique_ptr<drm::Shader>> Shaders;

	};
}