#pragma once
#include "GPU/GPUResource.h"
#include <vulkan/vulkan.h>
#include <typeindex>
#include <map>

/** Reflects C++ types to shader types. */
class ShaderTypeReflector
{
public:
	template<typename T>
	static std::string Reflect();
};

struct ShaderTypeReflectionInfo
{
	std::string type;
	std::string members;
	std::string structStr;
	uint32		size;
};

struct DescriptorSetReflectionInfo
{
	std::vector<DescriptorBinding> bindings;
};

struct DescriptorSetReflectionTask
{
	DescriptorSetReflectionInfo	reflectionInfo;
	VkDescriptorSet&			descriptorSet;
	VkDescriptorUpdateTemplate&	descriptorUpdateTemplate;
};

namespace gpu
{
	std::string& GetShaderTypeReflectionTasks();

	std::vector<DescriptorSetReflectionTask>& GetDescriptorSetReflectionTasks();
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
	static void ReflectMember(MemberId##Name memberId, std::string& members)									\
	{																											\
		members += ShaderTypeReflector::Reflect<ShaderType>() + " " + std::string(#Name) + ";";					\
		ReflectMember(NextMemberId##Name{}, members);															\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End push constant block. */
#define END_UNIFORM_BLOCK(StructName)																			\
		LastMemberId;																							\
	static void ReflectMember(LastMemberId memberId, std::string& members) {}									\
public:																											\
	static std::string ReflectMembers()																			\
	{																											\
		std::string members;																					\
		ReflectMember(FirstMemberId{}, members);																\
		return members;																							\
	}																											\
	struct ReflectionTask : ShaderTypeReflectionInfo{															\
		ReflectionTask() {																						\
			type = std::string(#StructName);																	\
			members = ReflectMembers();																			\
			size = sizeof(StructName);																			\
			structStr = "struct " + type + "{" + members + "};\n";												\
			auto& tasks = gpu::GetShaderTypeReflectionTasks();													\
			tasks += structStr;																					\
		}																										\
	};																											\
	static ReflectionTask _ReflectionTask;																		\
};																												\

#define DECLARE_UNIFORM_BLOCK(StructName)																		\
	StructName::ReflectionTask StructName::_ReflectionTask;														\

#define BEGIN_UNIFORM_BUFFER(StructName)	\
	BEGIN_UNIFORM_BLOCK(StructName)
#define END_UNIFORM_BUFFER(StructName)		\
	END_UNIFORM_BLOCK(StructName)

#define DECLARE_UNIFORM_BUFFER(StructName)  \
	DECLARE_UNIFORM_BLOCK(StructName)

#define BEGIN_PUSH_CONSTANTS(StructName)	\
	BEGIN_UNIFORM_BLOCK(StructName)
#define END_PUSH_CONSTANTS(StructName)		\
	END_UNIFORM_BLOCK(StructName)			\
	DECLARE_UNIFORM_BLOCK(StructName)		

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
	static void ReflectMember(MemberId##Name memberId, DescriptorSetReflectionInfo& reflectionInfo)				\
	{																											\
		DescriptorBinding binding;																				\
		binding.binding = static_cast<uint32>(reflectionInfo.bindings.size());									\
		binding.descriptorCount = 1;																			\
		binding.descriptorType = DescriptorType::GetDescriptorType();											\
		reflectionInfo.bindings.push_back(binding);																\
		ReflectMember(NextMemberId##Name{}, reflectionInfo);													\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End descriptor set declaration. */
#define END_DESCRIPTOR_SET(StructName)																			\
		LastMemberId;																							\
	static void ReflectMember(LastMemberId memberId, DescriptorSetReflectionInfo& reflectionInfo) {}			\
public:																											\
	static DescriptorSetReflectionInfo ReflectMembers()															\
	{																											\
		DescriptorSetReflectionInfo reflectionInfo;																\
		ReflectMember(FirstMemberId{}, reflectionInfo);															\
		return reflectionInfo;																					\
	}																											\
	struct ReflectionTask																						\
	{																											\
		ReflectionTask() {																						\
			auto& tasks = gpu::GetDescriptorSetReflectionTasks();												\
			tasks.push_back({ReflectMembers(),_DescriptorSet,_DescriptorUpdateTemplate});						\
		}																										\
	};																											\
	static ReflectionTask				_ReflectionTask;														\
	static VkDescriptorSet				_DescriptorSet;															\
	static VkDescriptorUpdateTemplate	_DescriptorUpdateTemplate;												\
};																												\

#define DECLARE_DESCRIPTOR_SET(StructName)																		\
	StructName::ReflectionTask	StructName::_ReflectionTask;													\
	VkDescriptorSet				StructName::_DescriptorSet;														\
	VkDescriptorUpdateTemplate	StructName::_DescriptorUpdateTemplate;											\

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