#include "VulkanShader.h"
#include "VulkanDRM.h"
#include <SPIRV-Cross/spirv_glsl.hpp>

VulkanShader::VulkanShader(VkShaderModule ShaderModule, const std::vector<VkVertexInputAttributeDescription>& Attributes)
	: ShaderModule(ShaderModule), Attributes(Attributes)
{
}

static const HashTable<EShaderStage, VkShaderStageFlagBits> VulkanStages =
{
	ENTRY(EShaderStage::Vertex, VK_SHADER_STAGE_VERTEX_BIT)
	ENTRY(EShaderStage::TessControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	ENTRY(EShaderStage::TessEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
	ENTRY(EShaderStage::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT)
	ENTRY(EShaderStage::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT)
	ENTRY(EShaderStage::Compute, VK_SHADER_STAGE_COMPUTE_BIT)
};

VkShaderStageFlagBits VulkanShader::GetVulkanStage(EShaderStage Stage)
{
	return VulkanStages.at(Stage);
}

struct VertexStreamFormat
{
	std::string Name;
	int32 Location = -1;
	VkFormat Format;
};

struct ResourceFormat
{
	std::string Name;
	int32 Binding = -1;
	int32 Set = -1;
};

static VkFormat GetFormatFromBaseType(const spirv_cross::SPIRType& Type)
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
}

static std::vector<VertexStreamFormat> ParseStageInputs(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<VertexStreamFormat> VertexStreams;

	for (auto& Resource : Resources.stage_inputs)
	{
		VertexStreamFormat VertexStream;
		VertexStream.Name = Resource.name;
		VertexStream.Location = GLSL.get_decoration(Resource.id, spv::DecorationLocation);
		VertexStream.Format = GetFormatFromBaseType(GLSL.get_type(Resource.type_id));

		VertexStreams.push_back(VertexStream);
	}

	return VertexStreams;
}

static std::vector<ResourceFormat> ParseSampledImages(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<ResourceFormat> Images;

	for (auto& Resource : Resources.sampled_images)
	{
		uint32 Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding);

		ResourceFormat Image;
		Image.Name = Resource.name;
		Image.Binding = Binding;
		Image.Set = GLSL.get_decoration(Resource.id, spv::DecorationDescriptorSet);

		Images.push_back(Image);
	}

	return Images;
}

static std::vector<ResourceFormat> ParseUniformBuffers(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<ResourceFormat> Uniforms;

	for (auto& Resource : Resources.uniform_buffers)
	{
		uint32 Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding);

		ResourceFormat Uniform;
		Uniform.Name = Resource.name;
		Uniform.Binding = Binding;
		Uniform.Set = GLSL.get_decoration(Resource.id, spv::DecorationDescriptorSet);

		size_t Size = (uint32)GLSL.get_declared_struct_size(GLSL.get_type(Resource.type_id));
		check(Size % 16 == 0, "std140 layout advises to manually pad structures/arrays to multiple of 16 bytes.");

		Uniforms.push_back(Uniform);
	}

	return Uniforms;
}

static std::vector<ResourceFormat> ParseStorageBuffers(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<ResourceFormat> StorageBuffers;

	for (auto& Resource : Resources.storage_buffers)
	{
		uint32 Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding);

		ResourceFormat StorageBuffer;
		StorageBuffer.Name = Resource.name;
		StorageBuffer.Binding = Binding;
		StorageBuffer.Set = GLSL.get_decoration(Resource.id, spv::DecorationDescriptorSet);

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

static HashTable<std::string, SpecConstant> ParseSpecializationConstants(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	HashTable<std::string, SpecConstant> SpecConstants;
	std::vector<spirv_cross::SpecializationConstant> SPIRVSpecializationConstants = GLSL.get_specialization_constants();

	for (auto& SPIRSpecConstant : SPIRVSpecializationConstants)
	{
		const std::string Name = GLSL.get_name(SPIRSpecConstant.id);
		SpecConstants[Name] = SpecConstant(SPIRSpecConstant.constant_id);
	}

	return SpecConstants;
}

ShaderResourceTable VulkanDRM::CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
{
	static const std::string ShaderCompilerPath = "C:/VulkanSDK/1.1.73.0/Bin32/glslc.exe";
	static const std::string SPIRVExt = ".spv";

	// Not a great approach. 
	const std::string ShaderExt = [&] ()
	{
		switch (Meta.Stage)
		{
		case EShaderStage::Vertex:
			return "vert";
		case EShaderStage::TessControl:
			return "tesc";
		case EShaderStage::TessEvaluation:
			return "tese";
		case EShaderStage::Geometry:
			return "geom";
		case EShaderStage::Fragment:
			return "frag";
		default: // Compute
			return "comp";
		}
	}();

	std::stringstream SS;

	std::for_each(Worker.GetDefines().begin(), Worker.GetDefines().end(), [&] (const std::pair<std::string, std::string>& Defines)
	{
		const auto& [Define, Value] = Defines;
		SS << " -D" + Define + "=" + Value;
	});
	
	SS << " -std=450";
	SS << " -fshader-stage=" + ShaderExt;
	SS << " -fentry-point=" + Meta.EntryPoint;
	SS << " -o ";
	SS << Meta.Filename + SPIRVExt;
	SS << " " + Meta.Filename;

	while (true)
	{
		Platform.ForkProcess(ShaderCompilerPath, SS.str());

		// Hack until ForkProcess can return STDOUT of child process.
		if (Platform.FileExists(Meta.Filename + SPIRVExt))
		{
			break;
		}

		const EMBReturn Ret = Platform.DisplayMessageBox(EMBType::RETRYCANCEL, EMBIcon::WARNING, "Shader failed to compile. Filename: " + Meta.Filename, "Shader Compiler Error");

		switch (Ret)
		{
		case EMBReturn::CANCEL:
			Platform.Exit();
		case EMBReturn::RETRY:
			LOG("Recompiling shader %s", Meta.Filename.c_str());
		}
	}

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
	std::vector<ResourceFormat> Uniforms = ParseUniformBuffers(GLSL, Resources);
	std::vector<ResourceFormat> Images = ParseSampledImages(GLSL, Resources);
	std::vector<ResourceFormat> StorageBuffers = ParseStorageBuffers(GLSL, Resources);
	HashTable<std::string, SpecConstant> SpecConstants = ParseSpecializationConstants(GLSL, Resources);

	std::vector<VkVertexInputAttributeDescription> Attributes = CreateVertexInputAttributeDescriptions(VertexStreams);
	VkShaderStageFlags Stage = VulkanStages.at(Meta.Stage);

	HashTable<std::string, ShaderBinding> ShaderBindings;

	std::for_each(Uniforms.begin(), Uniforms.end(), [&] (const ResourceFormat& Uniform)
	{
		ShaderBindings[Uniform.Name] = ShaderBinding(Uniform.Binding);
	});

	std::for_each(Images.begin(), Images.end(), [&] (const ResourceFormat& Image)
	{
		ShaderBindings[Image.Name] = ShaderBinding(Image.Binding);
	});

	std::for_each(StorageBuffers.begin(), StorageBuffers.end(), [&](const ResourceFormat& StorageBuffer)
	{
		ShaderBindings[StorageBuffer.Name] = ShaderBinding(StorageBuffer.Binding);
	});
	
	Device.ShaderCache.emplace(Meta.Type, VulkanShader{ ShaderModule, Attributes });

	return ShaderResourceTable(Meta.Type, Meta.Stage, Meta.EntryPoint, ShaderBindings, SpecConstants);
}