#include "VulkanShaderLibrary.h"
#include "VulkanDevice.h"
#include <SPIRV-Cross/spirv_glsl.hpp>
#include <shaderc/shaderc.hpp>
#include <unordered_map>

static VkShaderStageFlags TranslateStageFlags(EShaderStage stageFlags)
{
	VkShaderStageFlags vulkanStageFlags = 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::TessControl) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::TessEvaluation) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::Geometry) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	vulkanStageFlags |= Any(stageFlags & EShaderStage::Compute) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return vulkanStageFlags;
}

static VkFormat GetFormatFromBaseType(const spirv_cross::SPIRType& type)
{
	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Int:
		switch (type.width)
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
		switch (type.width)
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
		switch (type.vecsize)
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

static std::vector<VertexAttributeDescription> ReflectVertexAttributeDescriptions(
	const spirv_cross::CompilerGLSL& glsl, 
	const spirv_cross::ShaderResources& resources)
{
	std::vector<VertexAttributeDescription> descriptions;

	for (auto& resource : resources.stage_inputs)
	{
		VertexAttributeDescription description = {};
		description.location = glsl.get_decoration(resource.id, spv::DecorationLocation);
		description.format = VulkanImage::GetEngineFormat(GetFormatFromBaseType(glsl.get_type(resource.type_id)));
		description.offset = 0;

		descriptions.push_back(description);
	}

	// This sorting saves from having to figure out
	// a mapping between layout(location = ...) and buffer binding point
	std::sort(descriptions.begin(), descriptions.end(),
		[] (const VertexAttributeDescription& lhs, const VertexAttributeDescription& rhs)
	{
		return lhs.location < rhs.location;
	});

	for (uint32_t binding = 0; binding < descriptions.size(); binding++)
	{
		descriptions[binding].binding = binding;
	}

	return descriptions;
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

	return {};
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
		static const std::string shaderPath = "../Shaders/";

		auto& sourceName = _SourceNames.emplace_back(shaderPath + std::string(requestedSource));

		if (auto iter = _Includes.find(sourceName); iter != _Includes.end())
		{
			return &iter->second;
		}
		else
		{
			auto& source = _Sources.emplace_back(Platform::FileRead(sourceName));
			auto include = _Includes.emplace(sourceName, shaderc_include_result{ sourceName.c_str(), sourceName.size(), source.c_str(), source.size() });
			return &include.first->second;
		}
	}

	void ReleaseInclude(shaderc_include_result* data) override {}

private:
	std::list<std::string> _SourceNames;
	std::list<std::string> _Sources;
	std::unordered_map<std::string, shaderc_include_result> _Includes;
};

VulkanShaderLibrary::VulkanShaderLibrary(VulkanDevice& device)
	: _Device(device)
{
}

ShaderCompilationInfo VulkanShaderLibrary::CompileShader(
	const ShaderCompilerWorker& worker,
	const std::string& filename,
	const std::string& entryPoint,
	EShaderStage stage,
	std::type_index type
)
{
	shaderc::CompileOptions compileOptions;
	compileOptions.SetForcedVersionProfile(450, shaderc_profile::shaderc_profile_none);
	compileOptions.SetIncluder(std::make_unique<ShadercIncluder>());

	const shaderc_shader_kind shaderKind = [&] ()
	{
		switch (stage)
		{
		case EShaderStage::Vertex:
			compileOptions.AddMacroDefinition("VERTEX_SHADER", "1");
			return shaderc_vertex_shader;
		case EShaderStage::TessControl:
			compileOptions.AddMacroDefinition("TESSCONTROL_SHADER", "1");
			return shaderc_tess_control_shader;
		case EShaderStage::TessEvaluation:
			compileOptions.AddMacroDefinition("TESSEVAL_SHADER", "1");
			return shaderc_tess_evaluation_shader;
		case EShaderStage::Geometry:
			compileOptions.AddMacroDefinition("GEOMETRY_SHADER", "1");
			return shaderc_geometry_shader;
		case EShaderStage::Fragment:
			compileOptions.AddMacroDefinition("FRAGMENT_SHADER", "1");
			return shaderc_fragment_shader;
		default: // Compute
			return shaderc_compute_shader;
		}
	}();

	for (const auto& [define, value] : worker.GetDefines())
	{
		compileOptions.AddMacroDefinition(define, value);
	}

	const auto pushConstantDecl = [&] () -> std::string
	{
		if (!worker.GetPushConstantMembers().empty())
		{
			return "layout(push_constant) uniform _ShaderStruct{" + std::string(worker.GetPushConstantMembers()) + "};\n";
		}
		
		return "";
	}();

	const std::string globalShaderStructs = pushConstantDecl + gShaderStructs;

	shaderc::Compiler compiler;
	shaderc::SpvCompilationResult spvCompilationResult;

	do
	{
		const std::string sourceText = Platform::FileRead(filename, globalShaderStructs);

		spvCompilationResult = compiler.CompileGlslToSpv(sourceText, shaderKind, filename.c_str(), entryPoint.c_str(), compileOptions);

		if (spvCompilationResult.GetNumErrors() > 0)
		{
			const EMBReturn ret = Platform::DisplayMessageBox(
				EMBType::RETRYCANCEL, EMBIcon::WARNING, 
				"Failed to compile " + filename + "\n\n" + spvCompilationResult.GetErrorMessage().c_str(), "Shader Compiler"
			);

			if (ret == EMBReturn::CANCEL)
			{
				Platform::Exit();
			}
		}
	} while (spvCompilationResult.GetNumErrors() > 0);

	std::vector<uint32> code(spvCompilationResult.begin(), spvCompilationResult.end());

	VkShaderModuleCreateInfo shaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shaderModuleCreateInfo.codeSize = code.size() * sizeof(uint32);
	shaderModuleCreateInfo.pCode = code.data();

	VkShaderModule shaderModule;
	vulkan(vkCreateShaderModule(_Device, &shaderModuleCreateInfo, nullptr, &shaderModule));

	const spirv_cross::CompilerGLSL glsl(code.data(), code.size());
	const spirv_cross::ShaderResources resources = glsl.get_shader_resources();

	const auto vertexAttributeDescriptions = [&] () -> std::vector<VertexAttributeDescription>
	{
		if (stage == EShaderStage::Vertex)
		{
			return ReflectVertexAttributeDescriptions(glsl, resources);
		}

		return {};
	}();

	const auto layouts = ReflectDescriptorSetLayouts(_Device, glsl, resources);

	const auto vulkanStageFlags = TranslateStageFlags(stage);

	const auto pushConstantRange = [&] () -> VkPushConstantRange
	{
		if (worker.GetPushConstantSize() == 0)
		{
			return ReflectPushConstantRange(glsl, resources, vulkanStageFlags);
		}

		return { vulkanStageFlags, worker.GetPushConstantOffset(), worker.GetPushConstantSize() };
	}();

	return ShaderCompilationInfo(
		type, stage, entryPoint, filename, Platform::GetLastWriteTime(filename), worker,
		shaderModule, vertexAttributeDescriptions, layouts, pushConstantRange);
}

void VulkanShaderLibrary::RecompileShaders()
{
	for (const auto& [shaderType, shader] : _Shaders)
	{
		const ShaderCompilationInfo& compileInfo = shader->compilationInfo;
		const uint64 lastWriteTime = Platform::GetLastWriteTime(compileInfo.filename);

		if (lastWriteTime > compileInfo.lastWriteTime)
		{
			// Destroy the old shader module.
			vkDestroyShaderModule(_Device, compileInfo.shaderModule, nullptr);

			shader->compilationInfo = CompileShader(
				compileInfo.worker,
				compileInfo.filename,
				compileInfo.entrypoint,
				compileInfo.stage,
				compileInfo.type);
		}
	}

	_Device.GetCache().RecompilePipelines();
}