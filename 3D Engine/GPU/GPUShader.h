#pragma once
#include "GPU/GPUResource.h"
#include <vulkan/vulkan.h>
#include <typeindex>
#include <map>

/** Serializes C++ types to shader types. */
class ShaderTypeSerializer
{
public:
	template<typename T>
	static std::string Serialize();
};

struct UniformBlockSerialized
{
	std::string type;
	std::string members;
	std::string structStr;
	uint32 size;
};

struct PushConstantSerialized : public UniformBlockSerialized
{
};

struct UniformBufferSerialized : public UniformBlockSerialized
{
};

extern std::string gShaderStructs;

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
#define END_UNIFORM_BLOCK(StructName, SerializedStruct)															\
		LastMemberId;																							\
	static void Serialize(LastMemberId memberId, std::string& members) {}										\
public:																											\
	static std::string Serialize()																				\
	{																											\
		std::string members;																					\
		Serialize(FirstMemberId{}, members);																	\
		return members;																							\
	}																											\
	struct SerializedData : SerializedStruct{																	\
		SerializedData() {																						\
			type = std::string(#StructName);																	\
			members = Serialize();																				\
			size = sizeof(StructName);																			\
			structStr = "struct " + type + "{" + members + "};\n";												\
			gShaderStructs += structStr;																		\
		}																										\
	};																											\
	static SerializedData decl;																					\
};																												\
StructName::SerializedData StructName::decl;																	\

struct DescriptorsSerialized
{
	std::vector<DescriptorBinding> bindings;
};

/** Begin a descriptor set declaration. */
#define BEGIN_DESCRIPTOR_SET(StructName)\
struct StructName						\
{										\
public:									\
	StructName() = default;				\
private:								\
	struct FirstMemberId {};			\
	typedef FirstMemberId				\

/** Declare a descriptor binding in a descriptor set. */
#define DESCRIPTOR(DescriptorType, Name)																		\
		MemberId##Name;																							\
public:																											\
	DescriptorType Name;																						\
private:																										\
	static void Serialize(MemberId##Name memberId, DescriptorsSerialized& data)									\
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
	static void Serialize(LastMemberId memberId, DescriptorsSerialized& data) {}								\
public:																											\
	static DescriptorsSerialized Serialize()																	\
	{																											\
		DescriptorsSerialized data;																				\
		Serialize(FirstMemberId{}, data);																		\
		return data;																							\
	}																											\
	struct SerializedData : public DescriptorsSerialized 														\
	{																											\
		SerializedData() {																						\
			auto data = Serialize();																			\
			bindings = std::move(data.bindings);																\
		}																										\
	};																											\
	static SerializedData& GetSerialized()																		\
	{																											\
		static SerializedData data;																				\
		return data;																							\
	}																											\
	static gpu::DescriptorSetLayout& GetLayout(gpu::Device& device)												\
	{																											\
		static gpu::DescriptorSetLayout layout( device.CreateDescriptorSetLayout( GetSerialized().bindings.size(), GetSerialized().bindings.data() ) ); \
		return layout;																							\
	}																											\
};																												\

#define BEGIN_UNIFORM_BUFFER(StructName) \
	BEGIN_UNIFORM_BLOCK(StructName)
#define END_UNIFORM_BUFFER(StructName) \
	END_UNIFORM_BLOCK(StructName, UniformBufferSerialized)

#define BEGIN_PUSH_CONSTANTS(StructName) \
	BEGIN_UNIFORM_BLOCK(StructName)	
#define END_PUSH_CONSTANTS(StructName) \
	END_UNIFORM_BLOCK(StructName, PushConstantSerialized)

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
	inline void operator<<(const PushConstantSerialized& pushConstantSerialized)
	{
		_PushConstantMembers = pushConstantSerialized.members;
		_PushConstantSize = pushConstantSerialized.size;
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
	VkShaderModule shaderModule;
	std::vector<VertexAttributeDescription> vertexAttributeDescriptions;
	std::map<uint32, VkDescriptorSetLayout> layouts;
	VkPushConstantRange pushConstantRange;

	ShaderCompilationInfo(
		std::type_index type,
		EShaderStage stage, 
		const std::string& entrypoint,
		const std::string& filename,
		uint64 lastWriteTime,
		const ShaderCompilerWorker& worker,
		VkShaderModule shaderModule,
		const std::vector<VertexAttributeDescription>& vertexAttributeDescriptions,
		const std::map<uint32, VkDescriptorSetLayout>& layouts,
		const VkPushConstantRange& pushConstantRange)
		: type(type)
		, stage(stage)
		, entrypoint(entrypoint)
		, filename(filename)
		, lastWriteTime(lastWriteTime)
		, worker(worker)
		, shaderModule(shaderModule)
		, vertexAttributeDescriptions(vertexAttributeDescriptions)
		, layouts(layouts)
		, pushConstantRange(pushConstantRange)
	{
	}
};

namespace gpu
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
		std::unordered_map<std::type_index, std::unique_ptr<gpu::Shader>> _Shaders;

	};
}