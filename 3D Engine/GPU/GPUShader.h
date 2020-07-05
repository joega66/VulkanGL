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

	template<typename T>
	static EDescriptorType GetDescriptorType();
};

struct PushConstantSerializedData
{
	std::string decl;
	uint32 size;
};

/** Begin a push constant definition. */
#define BEGIN_PUSH_CONSTANTS(StructName)\
struct StructName						\
{										\
private:								\
	struct FirstMemberId {};			\
	typedef FirstMemberId				\

/** Define a member of a push constant. */
#define SHADER_PARAMETER(ShaderType, Name)																		\
		MemberId##Name;																							\
public:																											\
	ShaderType Name;																							\
private:																										\
	static void Serialize(MemberId##Name memberId, std::string& shaderStruct)									\
	{																											\
		shaderStruct += ShaderTypeSerializer::Serialize<ShaderType>() + " " + std::string(#Name) + ";";			\
		Serialize(NextMemberId##Name{}, shaderStruct);															\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End push constant definition. */
#define END_PUSH_CONSTANTS(StructName)																			\
		LastMemberId;																							\
	static void Serialize(LastMemberId memberId, std::string& shaderStruct) {}									\
public:																											\
	static std::string Serialize()																				\
	{																											\
		std::string shaderStruct;																				\
		Serialize(FirstMemberId{}, shaderStruct);																\
		return shaderStruct;																					\
	}																											\
	struct SerializedData##StructName : PushConstantSerializedData{												\
		SerializedData##StructName() {																			\
			decl = Serialize();																					\
			size = sizeof(StructName);																			\
		}																										\
	};																											\
	static SerializedData##StructName decl;																		\
};																												\
StructName::SerializedData##StructName StructName::decl;														\

/** Begin a descriptor set definition. */
#define BEGIN_DESCRIPTOR_SET(StructName)\
struct StructName						\
{										\
public:									\
	StructName() = default;				\
private:								\
	struct FirstMemberId {};			\
	typedef FirstMemberId				\

/** Define a member/binding of a descriptor set. */
#define DESCRIPTOR(DescriptorType, Name)																		\
		MemberId##Name;																							\
public:																											\
	DescriptorType Name;																						\
private:																										\
	static void Serialize(MemberId##Name memberId, std::vector<DescriptorBinding>& bindings)					\
	{																											\
		DescriptorBinding binding;																				\
		binding.binding = static_cast<uint32>(bindings.size());													\
		binding.descriptorCount = 1;																			\
		binding.descriptorType = ShaderTypeSerializer::GetDescriptorType<DescriptorType>();						\
		bindings.push_back(binding);																			\
		Serialize(NextMemberId##Name{}, bindings);																\
	}																											\
	struct NextMemberId##Name {};																				\
	typedef NextMemberId##Name																					\

/** End descriptor set definition. */
#define END_DESCRIPTOR_SET(StructName)																			\
		LastMemberId;																							\
	static void Serialize(LastMemberId memberId, std::vector<DescriptorBinding>& bindings) {}					\
public:																											\
	static std::vector<DescriptorBinding> Serialize()															\
	{																											\
		std::vector<DescriptorBinding> bindings;																\
		Serialize(FirstMemberId{}, bindings);																	\
		return bindings;																						\
	}																											\
	struct SerializedData																						\
	{																											\
		std::vector<DescriptorBinding> bindings;																\
		SerializedData() {																						\
			bindings = Serialize();																				\
		}																										\
	};																											\
	static gpu::DescriptorSetLayout& GetLayout(gpu::Device& device)												\
	{																											\
		static gpu::DescriptorSetLayout layout( device.CreateDescriptorSetLayout( _Serialized.bindings.size(), _Serialized.bindings.data() ) ); \
		return layout;																							\
	}																											\
private:																										\
	static SerializedData _Serialized;																			\
};																												\

#define DECLARE_DESCRIPTOR_SET(StructName) \
	StructName::SerializedData StructName::_Serialized;	\

#define END_DESCRIPTOR_SET_STATIC(StructName)			\
	END_DESCRIPTOR_SET(StructName)						\
	DECLARE_DESCRIPTOR_SET(StructName)					\

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
	inline void operator<<(const PushConstantSerializedData& shaderStructDecl)
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