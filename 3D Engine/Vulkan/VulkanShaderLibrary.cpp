#include "VulkanShaderLibrary.h"
#include "VulkanDevice.h"
#include <SPIRV-Cross/spirv_glsl.hpp>
#include <shaderc/shaderc.hpp>
#include <unordered_map>

static VkShaderStageFlags TranslateStageFlags(EShaderStage stageFlags)
{
	VkShaderStageFlags VkStageFlags = 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::TessControl) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::TessEvaluation) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Geometry) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	VkStageFlags |= Any(stageFlags & EShaderStage::Compute) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return VkStageFlags;
}

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

static std::vector<VertexAttributeDescription> ReflectVertexAttributeDescriptions(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<VertexAttributeDescription> Descriptions;

	for (auto& Resource : Resources.stage_inputs)
	{
		VertexAttributeDescription Description = {};
		Description.location = GLSL.get_decoration(Resource.id, spv::DecorationLocation);
		Description.format = VulkanImage::GetEngineFormat(GetFormatFromBaseType(GLSL.get_type(Resource.type_id)));
		Description.offset = 0;

		Descriptions.push_back(Description);
	}

	// This sorting saves from having to figure out
	// a mapping between layout(location = ...) and buffer binding point
	std::sort(Descriptions.begin(), Descriptions.end(),
		[] (const VertexAttributeDescription& LHS, const VertexAttributeDescription& RHS)
	{
		return LHS.location < RHS.location;
	});

	for (uint32_t Binding = 0; Binding < Descriptions.size(); Binding++)
	{
		Descriptions[Binding].binding = Binding;
	}

	return Descriptions;
}

static std::map<uint32, VkDescriptorSetLayout> ReflectDescriptorSetLayouts(
	gpu::Device& device, 
	const spirv_cross::CompilerGLSL& glsl,
	const spirv_cross::ShaderResources& resources)
{
	std::map<uint32, VkDescriptorSetLayout> layouts;
	std::map<uint32, std::vector<DescriptorBinding>> setBindings;

	for (const auto& resource : resources.separate_images)
	{
		const spirv_cross::SPIRType& type = glsl.get_type(resource.type_id);
		const uint32 set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);

		if (type.array.size())
		{
			layouts.insert({ set, device.GetTextures().GetLayout() });
		}
		else
		{
			// Only bindless resources use separate images.
			signal_unimplemented();
		}
	}

	for (const auto& resource : resources.separate_samplers)
	{
		const spirv_cross::SPIRType& type = glsl.get_type(resource.type_id);
		const uint32 set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);

		if (type.array.size())
		{
			layouts.insert({ set, device.GetSamplers().GetLayout() });
		}
		else
		{
			// Only bindless resources use separate samplers.
			signal_unimplemented();
		}
	}

	auto getBindings = [&] (const auto& resources, EDescriptorType descriptorType)
	{
		for (const auto& resource : resources)
		{
			const spirv_cross::SPIRType& type = glsl.get_type(resource.type_id);
			const uint32 set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32 binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
			const uint32 descriptorCount = type.array.size() ? type.array[0] : 1;
			setBindings[set].push_back({ binding, descriptorCount, descriptorType });
		}
	};

	getBindings(resources.sampled_images, EDescriptorType::SampledImage);
	getBindings(resources.storage_images, EDescriptorType::StorageImage);
	getBindings(resources.uniform_buffers, EDescriptorType::UniformBuffer);
	getBindings(resources.storage_buffers, EDescriptorType::StorageBuffer);

	for (auto& [set, bindings] : setBindings)
	{
		layouts.insert({ set, device.CreateDescriptorSetLayout(bindings.size(), bindings.data()) });
	}

	return layouts;
}

static VkPushConstantRange ReflectPushConstantRange(
	const spirv_cross::CompilerGLSL& glsl,
	const spirv_cross::ShaderResources& resources,
	VkShaderStageFlags stageFlags)
{
	for (const auto& pushConstant : resources.push_constant_buffers)
	{
		const auto ranges = glsl.get_active_buffer_ranges(pushConstant.id);
		const spirv_cross::SPIRType& type = glsl.get_type(pushConstant.base_type_id);
		const std::size_t size = glsl.get_declared_struct_size(type);
		return { stageFlags, static_cast<uint32>(ranges.front().offset), static_cast<uint32>(size) };
	}

	return VkPushConstantRange{};
}

class ShadercIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	shaderc_include_result* GetInclude(
		const char* requestedSource,
		shaderc_include_type type,
		const char* requestingSource,
		size_t includeDepth) override
	{
		static const std::string ShaderPath = "../Shaders/";

		auto& SourceName = SourceNames.emplace_back(ShaderPath + std::string(requestedSource));

		if (auto Iter = Includes.find(SourceName); Iter != Includes.end())
		{
			return &Iter->second;
		}
		else
		{
			auto& Source = Sources.emplace_back(Platform::FileRead(SourceName));
			auto Include = Includes.emplace(SourceName, shaderc_include_result{ SourceName.c_str(), SourceName.size(), Source.c_str(), Source.size() });
			return &Include.first->second;
		}
	}

	void ReleaseInclude(shaderc_include_result* data) override {}

private:
	std::list<std::string> SourceNames;
	std::list<std::string> Sources;
	std::unordered_map<std::string, shaderc_include_result> Includes;
};

VulkanShaderLibrary::VulkanShaderLibrary(VulkanDevice& Device)
	: Device(Device)
{
}

ShaderCompilationInfo VulkanShaderLibrary::CompileShader(
	const ShaderCompilerWorker& Worker,
	const std::string& Filename,
	const std::string& EntryPoint,
	EShaderStage Stage,
	std::type_index Type
)
{
	auto Includer = std::make_unique<ShadercIncluder>();

	shaderc::CompileOptions CompileOptions;
	CompileOptions.SetForcedVersionProfile(450, shaderc_profile::shaderc_profile_none);
	CompileOptions.SetIncluder(std::move(Includer));

	const shaderc_shader_kind ShaderKind = [&] ()
	{
		switch (Stage)
		{
		case EShaderStage::Vertex:
			CompileOptions.AddMacroDefinition("VERTEX_SHADER", "1");
			return shaderc_vertex_shader;
		case EShaderStage::TessControl:
			CompileOptions.AddMacroDefinition("TESSCONTROL_SHADER", "1");
			return shaderc_tess_control_shader;
		case EShaderStage::TessEvaluation:
			CompileOptions.AddMacroDefinition("TESSEVAL_SHADER", "1");
			return shaderc_tess_evaluation_shader;
		case EShaderStage::Geometry:
			CompileOptions.AddMacroDefinition("GEOMETRY_SHADER", "1");
			return shaderc_geometry_shader;
		case EShaderStage::Fragment:
			CompileOptions.AddMacroDefinition("FRAGMENT_SHADER", "1");
			return shaderc_fragment_shader;
		default: // Compute
			return shaderc_compute_shader;
		}
	}();

	for (const auto& [Define, Value] : Worker.GetDefines())
	{
		CompileOptions.AddMacroDefinition(Define, Value);
	}

	const std::string pushConstantDecl = Worker.GetPushConstantMembers().empty() == false ?
		"layout(push_constant) uniform _ShaderStruct{" + std::string(Worker.GetPushConstantMembers()) + "};\n"
		: "";
	
	shaderc::Compiler Compiler;
	shaderc::SpvCompilationResult SpvCompilationResult;

	do
	{
		const std::string SourceText = Platform::FileRead(Filename, pushConstantDecl + gShaderStructs);

		SpvCompilationResult = Compiler.CompileGlslToSpv(SourceText, ShaderKind, Filename.c_str(), EntryPoint.c_str(), CompileOptions);

		if (SpvCompilationResult.GetNumErrors() > 0)
		{
			const EMBReturn Ret = Platform::DisplayMessageBox(
				EMBType::RETRYCANCEL, EMBIcon::WARNING, 
				"Failed to compile " + Filename + "\n\n" + SpvCompilationResult.GetErrorMessage().c_str(), "Shader Compiler"
			);

			if (Ret == EMBReturn::CANCEL)
			{
				Platform::Exit();
			}
		}
	} while (SpvCompilationResult.GetNumErrors() > 0);

	std::vector<uint32> Code(SpvCompilationResult.begin(), SpvCompilationResult.end());

	VkShaderModuleCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	CreateInfo.codeSize = Code.size() * sizeof(uint32);
	CreateInfo.pCode = Code.data();

	VkShaderModule ShaderModule;
	vulkan(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));

	spirv_cross::CompilerGLSL GLSL(Code.data(), Code.size());
	spirv_cross::ShaderResources Resources = GLSL.get_shader_resources();

	const std::vector<VertexAttributeDescription> VertexAttributeDescriptions = Stage == EShaderStage::Vertex ? 
		ReflectVertexAttributeDescriptions(GLSL, Resources) : std::vector<VertexAttributeDescription>{};

	const std::map<uint32, VkDescriptorSetLayout> layouts = ReflectDescriptorSetLayouts(Device, GLSL, Resources);

	const VkShaderStageFlags vulkanStageFlags = TranslateStageFlags(Stage);

	const VkPushConstantRange pushConstantRange = Worker.GetPushConstantSize() > 0 ? 
		VkPushConstantRange{ vulkanStageFlags, Worker.GetPushConstantOffset(), Worker.GetPushConstantSize() }
		: ReflectPushConstantRange(GLSL, Resources, vulkanStageFlags);

	return ShaderCompilationInfo(
		Type, Stage, EntryPoint, Filename, Platform::GetLastWriteTime(Filename), Worker,
		ShaderModule, VertexAttributeDescriptions, layouts, pushConstantRange);
}

void VulkanShaderLibrary::RecompileShaders()
{
	for (const auto& [ShaderType, Shader] : _Shaders)
	{
		const ShaderCompilationInfo& CompileInfo = Shader->compilationInfo;
		const uint64 LastWriteTime = Platform::GetLastWriteTime(CompileInfo.filename);

		if (LastWriteTime > CompileInfo.lastWriteTime)
		{
			// Destroy the old shader module.
			vkDestroyShaderModule(Device, CompileInfo.shaderModule, nullptr);

			Shader->compilationInfo = CompileShader(
				CompileInfo.worker,
				CompileInfo.filename, 
				CompileInfo.entrypoint,
				CompileInfo.stage, 
				CompileInfo.type);
		}
	}

	Device.GetCache().RecompilePipelines();
}