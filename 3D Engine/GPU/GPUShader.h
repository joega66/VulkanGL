#pragma once
#include "GPU/GPUResource.h"
#include <vulkan/vulkan.h>
#include <typeindex>
#include <map>

namespace gpu
{
	class DescriptorSetLayout;
}

/** Serializes C++ types to shader types. */
class ShaderTypeSerializer
{
public:
	template<typename T>
	static std::string Serialize();
};

/** Shader struct serialization data. */
struct ShaderStructSerialized
{
	std::string type;
	std::string members;
	std::string structStr;
	uint32		size;
};

/** Descriptor set serialization data. */
struct DescriptorSetSerialized
{
	std::vector<DescriptorBinding> bindings;
};

struct RegisteredDescriptorSetType
{
	gpu::DescriptorSetLayout& layout;
	std::vector<DescriptorBinding>& bindings;
};

namespace gpu
{
	std::string& GetRegisteredShaderStructs();

	std::vector<RegisteredDescriptorSetType>& GetRegisteredDescriptorSetTypes();
}

#define BEGIN_UNIFORM_BLOCK(StructName)	\
struct StructName						\
{										\
private:								\
	struct FirstMemberId {};			\
	typedef FirstMemberId				\

/** Declare a member of a uniform block. */
#define MEMBER(ShaderType, Name)																				\
		MemberId##Name;																							\
public:																											\
	ShaderType Name;																							\
private:																										\
	static void Serialize(MemberId##Name memberId, std::string& members)										\
	{																											\
		members += ShaderTypeSerializer::Serialize<ShaderType>() + " " + std::string(#Name) + ";";				\
		Serialize(NextMemberId##Name{}, members);																\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End push constant block. */
#define END_UNIFORM_BLOCK(StructName)																			\
		LastMemberId;																							\
	static void Serialize(LastMemberId memberId, std::string& members) {}										\
public:																											\
	static std::string Serialize()																				\
	{																											\
		std::string members;																					\
		Serialize(FirstMemberId{}, members);																	\
		return members;																							\
	}																											\
	struct SerializedData : ShaderStructSerialized{																\
		SerializedData() {																						\
			type = std::string(#StructName);																	\
			members = Serialize();																				\
			size = sizeof(StructName);																			\
			structStr = "struct " + type + "{" + members + "};\n";												\
			auto& registrar = gpu::GetRegisteredShaderStructs();												\
			registrar += structStr;																				\
		}																										\
	};																											\
	static SerializedData decl;																					\
};																												\
StructName::SerializedData StructName::decl;																	\

#define BEGIN_UNIFORM_BUFFER(StructName)	\
	BEGIN_UNIFORM_BLOCK(StructName)
#define END_UNIFORM_BUFFER(StructName)		\
	END_UNIFORM_BLOCK(StructName)

#define BEGIN_PUSH_CONSTANTS(StructName)	\
	BEGIN_UNIFORM_BLOCK(StructName)
#define END_PUSH_CONSTANTS(StructName)		\
	END_UNIFORM_BLOCK(StructName)

/** Begin a descriptor set declaration. */
#define BEGIN_DESCRIPTOR_SET(StructName)	\
struct StructName							\
{											\
public:										\
	StructName() = default;					\
private:									\
	struct FirstMemberId {};				\
	typedef FirstMemberId					\

/** Declare a descriptor binding in a descriptor set. */
#define DESCRIPTOR(DescriptorType, Name)																		\
		MemberId##Name;																							\
public:																											\
	DescriptorType Name;																						\
private:																										\
	static void Serialize(MemberId##Name memberId, DescriptorSetSerialized& data)								\
	{																											\
		DescriptorBinding binding;																				\
		binding.binding = static_cast<uint32>(data.bindings.size());											\
		binding.descriptorCount = 1;																			\
		binding.descriptorType = DescriptorType::GetDescriptorType();											\
		data.bindings.push_back(binding);																		\
		Serialize(NextMemberId##Name{}, data);																	\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End descriptor set declaration. */
#define END_DESCRIPTOR_SET(StructName)																			\
		LastMemberId;																							\
	static void Serialize(LastMemberId memberId, DescriptorSetSerialized& data) {}								\
public:																											\
	static DescriptorSetSerialized Serialize()																	\
	{																											\
		DescriptorSetSerialized data;																			\
		Serialize(FirstMemberId{}, data);																		\
		return data;																							\
	}																											\
	struct SerializedData : public DescriptorSetSerialized 														\
	{																											\
		SerializedData() {																						\
			auto data = Serialize();																			\
			bindings = std::move(data.bindings);																\
			auto& registrar = gpu::GetRegisteredDescriptorSetTypes();											\
			registrar.push_back({ StructName::layout, bindings });												\
		}																										\
	};																											\
	static SerializedData decl;																					\
	static gpu::DescriptorSetLayout layout;																		\
};																												\
StructName::SerializedData StructName::decl;																	\
gpu::DescriptorSetLayout StructName::layout;																	\

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
	
	/** Define the shader's push constant block. */
	inline void operator<<(const ShaderStructSerialized& shaderStructSerialized)
	{
		_PushConstantMembers = shaderStructSerialized.members;
		_PushConstantSize = shaderStructSerialized.size;
	}

	/** Get the shader's push constant struct. */
	inline uint32 GetPushConstantOffset() const { return _PushConstantOffset; }
	inline uint32 GetPushConstantSize() const { return _PushConstantSize; }
	inline std::string_view GetPushConstantMembers() const { return _PushConstantMembers; }

private:
	ShaderDefines _Defines;
	uint32 _PushConstantOffset = 0;
	uint32 _PushConstantSize = 0;
	std::string_view _PushConstantMembers;
};

struct ShaderCompilationTask
{
	std::type_index			typeIndex;
	gpu::Shader*			shader;
	std::filesystem::path	path;
	std::string				entrypoint;
	EShaderStage			stage;
	ShaderCompilerWorker	worker;
};

namespace gpu
{
	std::vector<ShaderCompilationTask>& GetShaderCompilationTasks();
}

/** Result of shader compilation. */
class ShaderCompilationResult
{
public:
	/** Path to the shader. */
	std::filesystem::path path;

	/** Shader entrypoint. */
	std::string entrypoint;

	/** Shader stage. */
	EShaderStage stage;

	/** Last time the shader file was written, Platform::GetLastWriteTime(). */
	uint64 lastWriteTime;

	/** Result of SetEnvironmentVariables. */
	ShaderCompilerWorker worker;

	/** Vulkan shader handle. */
	VkShaderModule shaderModule;

	/** (Vertex shader only.) Reflected vertex attribute descriptions. PSOs use this if none were provided in the PSO description. */
	std::vector<VertexAttributeDescription> vertexAttributeDescriptions;

	/** Reflected descriptor set layouts. */
	std::map<uint32, VkDescriptorSetLayout> layouts;

	/** Reflected push constant range. */
	VkPushConstantRange pushConstantRange;

	ShaderCompilationResult() = default;
	ShaderCompilationResult(const std::filesystem::path& path, const std::string& entrypoint, EShaderStage stage, uint64 lastWriteTime,
		const ShaderCompilerWorker& worker, VkShaderModule shaderModule, const std::vector<VertexAttributeDescription>& vertexAttributeDescriptions,
		const std::map<uint32, VkDescriptorSetLayout>& layouts, const VkPushConstantRange& pushConstantRange)
		: stage(stage), entrypoint(entrypoint), path(path), lastWriteTime(lastWriteTime), worker(worker), shaderModule(shaderModule)
		, vertexAttributeDescriptions(vertexAttributeDescriptions), layouts(layouts), pushConstantRange(pushConstantRange)
	{
	}
};

namespace gpu
{
	class Shader
	{
	public:
		ShaderCompilationResult compilationResult;

		Shader() = default;

		static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
		{
		}
	};

	/** The shader library compiles statically registered shaders and caches them by type index. */
	class ShaderLibrary
	{
	public:
		/** Find shader of ShaderType. */
		template<typename ShaderType>
		const ShaderType* FindShader()
		{
			const std::type_index typeIndex = std::type_index(typeid(ShaderType));
			return static_cast<ShaderType*>(_Shaders[typeIndex]);
		}

		/** Recompile cached shaders. */
		virtual void RecompileShaders() = 0;

	private:
		/** Compile the shader. */
		virtual ShaderCompilationResult CompileShader(
			const ShaderCompilerWorker& worker,
			const std::filesystem::path& path,
			const std::string& entrypoint,
			EShaderStage stage) = 0;

	protected:
		/** Cached shaders. */
		std::unordered_map<std::type_index, gpu::Shader*> _Shaders;
	};
}

#define REGISTER_SHADER(Type, Path, Entrypoint, Stage)	\
class __##Type##CompilationTask							\
{														\
public:													\
	__##Type##CompilationTask()							\
	{													\
		static Type shader;								\
		ShaderCompilationTask task = {					\
			.typeIndex = std::type_index(typeid(Type)), \
			.shader = &shader,							\
			.path = Path,								\
			.entrypoint = Entrypoint,					\
			.stage = Stage,								\
		};												\
		Type::SetEnvironmentVariables(task.worker);		\
		gpu::GetShaderCompilationTasks().push_back(task); \
	} \
	static __##Type##CompilationTask task; \
}; __##Type##CompilationTask __##Type##CompilationTask::task;