#include "VulkanShader.h"
#include "VulkanDRM.h"
#include <SPIRV-Cross/spirv_glsl.hpp>

VulkanShader::VulkanShader(
	VkShaderModule ShaderModule
	, const std::vector<VkVertexInputAttributeDescription>& Attributes
	, const std::vector<VkDescriptorSetLayoutBinding>& Bindings)
	: ShaderModule(ShaderModule), Attributes(Attributes), Bindings(Bindings)
{
}

static const VkShaderStageFlagBits VulkanStages[] =
{
	VK_SHADER_STAGE_VERTEX_BIT,
	VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	VK_SHADER_STAGE_GEOMETRY_BIT,
	VK_SHADER_STAGE_FRAGMENT_BIT,
	VK_SHADER_STAGE_COMPUTE_BIT
};

VkShaderStageFlagBits VulkanShader::GetVulkanStage(EShaderStage Stage)
{
	return VulkanStages[(int32)Stage];
}

struct VertexStreamFormat
{
	std::string Name;
	int32 Location = -1;
	VkFormat Format;
};

struct UniformBufferFormat
{
	std::string Name;
	int32 Binding = -1;
	uint32 Size = 0;
};

struct ResourceFormat
{
	std::string Name;
	int32 Binding = -1;
};

static std::vector<VertexStreamFormat> ParseStageInputs(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<VertexStreamFormat> VertexStreams;

	for (auto& Resource : Resources.stage_inputs)
	{
		const auto& Type = GLSL.get_type(Resource.type_id);

		VertexStreamFormat VertexStream;
		VertexStream.Name = Resource.name;
		VertexStream.Location = GLSL.get_decoration(Resource.id, spv::DecorationLocation);
		VertexStream.Format = [&] ()
		{
			switch (Type.basetype)
			{
			case spirv_cross::SPIRType::BaseType::Int:
				switch (Type.width)
				{
				case 1:
					return VK_FORMAT_R32_SINT;
				case 2:
					return VK_FORMAT_R32G32_SINT;
				case 3:
					return VK_FORMAT_R32G32B32_SINT;
				case 4:
					return VK_FORMAT_R32G32B32A32_SINT;
				}
			case spirv_cross::SPIRType::BaseType::UInt:
				switch (Type.width)
				{
				case 1:
					return VK_FORMAT_R32_UINT;
				case 2:
					return VK_FORMAT_R32G32_UINT;
				case 3:
					return VK_FORMAT_R32G32B32_UINT;
				case 4:
					return VK_FORMAT_R32G32B32A32_UINT;
				}
			case spirv_cross::SPIRType::BaseType::Float:
				switch (Type.vecsize)
				{
				case 1:
					return VK_FORMAT_R32_SFLOAT;
				case 2:
					return VK_FORMAT_R32G32_SFLOAT;
				case 3:
					return VK_FORMAT_R32G32B32_SFLOAT;
				case 4:
					return VK_FORMAT_R32G32B32A32_SFLOAT;
				}
			default:
				signal_unimplemented();
			}
		}();

		VertexStreams.push_back(VertexStream);
	}

	return VertexStreams;
}

static std::vector<ResourceFormat> ParseSampledImages(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<ResourceFormat> Images;

	for (auto& Resource : Resources.sampled_images)
	{
		uint32 Binding;
		if (Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding); Binding == 0)
		{
			// Resource not used... continue.
			continue;
		}

		ResourceFormat Image;
		Image.Name = Resource.name;
		Image.Binding = Binding;

		Images.push_back(Image);
	}

	return Images;
}

static std::vector<UniformBufferFormat> ParseUniformBuffers(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<UniformBufferFormat> Uniforms;

	for (auto& Resource : Resources.uniform_buffers)
	{
		uint32 Binding;
		if (Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding); Binding == 0)
		{
			// Resource not used... continue.
			continue;
		}

		UniformBufferFormat Uniform;
		Uniform.Name = Resource.name;
		Uniform.Binding = Binding;
		Uniform.Size = (uint32)GLSL.get_declared_struct_size(GLSL.get_type(Resource.type_id));

		check(Uniform.Size % 16 == 0, "std140 layout advises to manually pad structures/arrays to multiple of 16 bytes.");

		Uniforms.push_back(Uniform);
	}

	return Uniforms;
}

static std::vector<ResourceFormat> ParseStorageBuffers(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<ResourceFormat> StorageBuffers;

	for (auto& Resource : Resources.storage_buffers)
	{
		uint32 Binding;
		if (Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding); Binding == 0)
		{
			// Resource not used... continue.
			continue;
		}

		ResourceFormat StorageBuffer;
		StorageBuffer.Name = Resource.name;
		StorageBuffer.Binding = Binding;

		StorageBuffers.push_back(StorageBuffer);
	}

	return StorageBuffers;
}

static std::vector<VkVertexInputAttributeDescription> CreateVertexInputAttributeDescriptions(std::vector<VertexStreamFormat>& Streams)
{
	// This sorting saves from having to figure out
	// a mapping between layout(location = ...) and buffer binding point
	std::sort(Streams.begin(), Streams.end(),
		[] (const VertexStreamFormat& LHS, const VertexStreamFormat& RHS)
	{
		return LHS.Location < RHS.Location;
	});

	std::vector<VkVertexInputAttributeDescription> Descriptions(Streams.size());

	for (uint32 Binding = 0; Binding < Streams.size(); Binding++)
	{
		VkVertexInputAttributeDescription Description = {};
		Description.binding = Binding;
		Description.location = Streams[Binding].Location;
		Description.format = Streams[Binding].Format;
		Description.offset = 0;
		Descriptions[Binding] = Description;
	}

	return Descriptions;
}

template<typename DescriptorType>
static void CreateDescriptorSetLayoutBindings(
	const std::vector<DescriptorType>& Descriptors
	, std::vector<VkDescriptorSetLayoutBinding>& Bindings
	, VkDescriptorType Type
	, VkShaderStageFlags Stage)
{
	for (const auto& Descriptor : Descriptors)
	{
		VkDescriptorSetLayoutBinding Binding = {};
		Binding.binding = Descriptor.Binding;
		Binding.descriptorCount = 1;
		Binding.descriptorType = Type;
		Binding.stageFlags = Stage;

		Bindings.push_back(Binding);
	}
}

ShaderResourceTable VulkanDRM::CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
{
	static const std::string ShaderCompilerPath = "C:/VulkanSDK/1.1.73.0/Bin32/glslc.exe";
	static const std::string SPIRVExt = ".spv";
	std::string BaseBinding;

	// Not a great approach. 
	const std::string ShaderExt = [&] ()
	{
		switch (Meta.Stage)
		{
		case EShaderStage::Vertex:
			BaseBinding = "1";
			return "vert";
		case EShaderStage::TessControl:
			BaseBinding = "100";
			return "tesc";
		case EShaderStage::TessEvaluation:
			BaseBinding = "200";
			return "tese";
		case EShaderStage::Geometry:
			BaseBinding = "300";
			return "geom";
		case EShaderStage::Fragment:
			BaseBinding = "400";
			return "frag";
		default: // Compute
			BaseBinding = "0";
			return "comp";
		}
	}();

	std::stringstream SS;

	std::for_each(Worker.GetDefines().begin(), Worker.GetDefines().end(), [&] (const std::pair<std::string, std::string>& Defines)
	{
		const auto& [Define, Value] = Defines;
		SS << " -D" + Define + "[=[" + Value + "]]";
	});
	
	SS << " -std=450";
	SS << " -fshader-stage=" + ShaderExt;
	SS << " -fentry-point=" + Meta.EntryPoint;
	SS << " -o ";
	SS << Meta.Filename + SPIRVExt;
	SS << " " + Meta.Filename;
	SS << " -fauto-bind-uniforms";
	SS << " -fimage-binding-base " + ShaderExt + " " + BaseBinding;
	SS << " -fsampler-binding-base " + ShaderExt + " " + BaseBinding;
	SS << " -ftexture-binding-base " + ShaderExt + " " + BaseBinding;
	SS << " -fubo-binding-base " + ShaderExt + " " + BaseBinding;
	SS << " -fssbo-binding-base " + ShaderExt + " " + BaseBinding;

	Platform.ForkProcess(ShaderCompilerPath, SS.str());

	// Hack until ForkProcess can return STDOUT of child process.
	check(Platform.FileExists(Meta.Filename + SPIRVExt),
		"Shader failed to compile.\nFilename: %s", Meta.Filename.c_str());
	const std::string SPIRV = Platform.FileRead(Meta.Filename + SPIRVExt);
	Platform.FileDelete(Meta.Filename + SPIRVExt);

	VkShaderModuleCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	CreateInfo.codeSize = SPIRV.size();
	CreateInfo.pCode = reinterpret_cast<const uint32*>(SPIRV.data());

	VkShaderModule ShaderModule;
	vulkan(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));

	spirv_cross::CompilerGLSL GLSL(reinterpret_cast<const uint32*>(SPIRV.data()), SPIRV.size() / sizeof(uint32));
	spirv_cross::ShaderResources Resources = GLSL.get_shader_resources();

	std::vector<VertexStreamFormat> VertexStreams = ParseStageInputs(GLSL, Resources);
	std::vector<UniformBufferFormat> Uniforms = ParseUniformBuffers(GLSL, Resources);
	std::vector<ResourceFormat> Images = ParseSampledImages(GLSL, Resources);
	std::vector<ResourceFormat> StorageBuffers = ParseStorageBuffers(GLSL, Resources);

	std::vector<VkVertexInputAttributeDescription> Attributes = CreateVertexInputAttributeDescriptions(VertexStreams);
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	VkShaderStageFlags Stage = VulkanStages[(int32)Meta.Stage];

	CreateDescriptorSetLayoutBindings(Uniforms, Bindings, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Stage);
	CreateDescriptorSetLayoutBindings(Images, Bindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Stage);
	CreateDescriptorSetLayoutBindings(StorageBuffers, Bindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Stage);

	std::sort(Bindings.begin(), Bindings.end(),
		[] (const VkDescriptorSetLayoutBinding& LHS, const VkDescriptorSetLayoutBinding& RHS)
	{
		return LHS.binding < RHS.binding;
	});

	HashTable<std::string, uint32> AttributeLocations;

	if (Meta.Stage == EShaderStage::Vertex)
	{
		std::for_each(VertexStreams.begin(), VertexStreams.end(), [&] (const VertexStreamFormat& Stream)
		{
			AttributeLocations[Stream.Name] = Stream.Location;
		});
	}

	HashTable<std::string, uint32> UniformLocations;

	std::for_each(Uniforms.begin(), Uniforms.end(), [&] (const UniformBufferFormat& Uniform)
	{
		UniformLocations[Uniform.Name] = Uniform.Binding;
	});

	std::for_each(Images.begin(), Images.end(), [&] (const ResourceFormat& Image)
	{
		UniformLocations[Image.Name] = Image.Binding;
	});

	std::for_each(StorageBuffers.begin(), StorageBuffers.end(), [&](const ResourceFormat& StorageBuffer)
	{
		UniformLocations[StorageBuffer.Name] = StorageBuffer.Binding;
	});
	
	Device.ShaderCache.emplace(Meta.Type, VulkanShader{ ShaderModule, Attributes, Bindings });

	return ShaderResourceTable(Meta.Type, Meta.Stage, Meta.EntryPoint, UniformLocations);
}