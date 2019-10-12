#include "VulkanShader.h"
#include "VulkanDRM.h"
#include <SPIRV-Cross/spirv_glsl.hpp>

#include <filesystem>

VulkanShader::VulkanShader(const VulkanDevice* Device, VkShaderModule ShaderModule, const std::vector<VkVertexInputAttributeDescription>& Attributes)
	: Device(Device), ShaderModule(ShaderModule), Attributes(Attributes)
{
}

VulkanShader::~VulkanShader()
{
	vkDestroyShaderModule(*Device, ShaderModule, nullptr);
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

static void ParseBindings(HashTable<std::string, ShaderBinding>& ShaderBindings, const spirv_cross::CompilerGLSL& GLSL, const std::vector<spirv_cross::Resource>& Resources)
{
	for (auto& Resource : Resources)
	{
		uint32 Binding = GLSL.get_decoration(Resource.id, spv::DecorationBinding);
		ShaderBindings[Resource.name] = ShaderBinding(Binding);
	}
}

static std::vector<VkVertexInputAttributeDescription> ParseVertexInputAttributeDescriptions(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<VkVertexInputAttributeDescription> Descriptions;

	for (auto& Resource : Resources.stage_inputs)
	{
		VkVertexInputAttributeDescription Description = {};

		Description.binding = Descriptions.size();
		Description.location = GLSL.get_decoration(Resource.id, spv::DecorationLocation);
		Description.format = GetFormatFromBaseType(GLSL.get_type(Resource.type_id));
		Description.offset = 0;

		Descriptions.push_back(Description);
	}

	// This sorting saves from having to figure out
	// a mapping between layout(location = ...) and buffer binding point
	std::sort(Descriptions.begin(), Descriptions.end(),
		[] (const VkVertexInputAttributeDescription& LHS, const VkVertexInputAttributeDescription& RHS)
	{
		return LHS.location < RHS.location;
	});

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

ShaderCompilationInfo VulkanDRM::CompileShader(const ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
{
	static const std::string ShaderCompilerPath = "../Shaders/glslc.exe";
	static const std::string SPIRVExt = ".spv";

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

	const uint64 LastWriteTime = Platform.GetLastWriteTime(Meta.Filename);

	const std::string SPIRV = Platform.FileRead(Meta.Filename + SPIRVExt);
	Platform.FileDelete(Meta.Filename + SPIRVExt);

	VkShaderModuleCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	CreateInfo.codeSize = SPIRV.size();
	CreateInfo.pCode = reinterpret_cast<const uint32*>(SPIRV.data());

	VkShaderModule ShaderModule;
	vulkan(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));

	spirv_cross::CompilerGLSL GLSL(reinterpret_cast<const uint32*>(SPIRV.data()), SPIRV.size() / sizeof(uint32));
	spirv_cross::ShaderResources Resources = GLSL.get_shader_resources();

	HashTable<std::string, ShaderBinding> ShaderBindings;
	ParseBindings(ShaderBindings, GLSL, Resources.uniform_buffers);
	ParseBindings(ShaderBindings, GLSL, Resources.sampled_images);
	ParseBindings(ShaderBindings, GLSL, Resources.storage_buffers);

	const HashTable<std::string, SpecConstant> SpecConstants = ParseSpecializationConstants(GLSL, Resources);

	const std::vector<VkVertexInputAttributeDescription> Attributes = ParseVertexInputAttributeDescriptions(GLSL, Resources);

	Device.ShaderCache[Meta.Type] = std::make_unique<VulkanShader>(&Device, ShaderModule, Attributes);
	
	return ShaderCompilationInfo(Meta.Type, Meta.Stage, Meta.EntryPoint, Meta.Filename, ShaderBindings, SpecConstants, LastWriteTime, Worker);
}

void VulkanDRM::RecompileShaders()
{
	for (const auto& [ShaderType, Shader] : Shaders)
	{
		const ShaderCompilationInfo& CompileInfo = Shader->CompilationInfo;

		const uint64 LastWriteTime = Platform.GetLastWriteTime(CompileInfo.Filename);

		if (LastWriteTime > CompileInfo.LastWriteTime)
		{
			LOG("Recompiling shader %s", CompileInfo.Filename.c_str());

			// The old VkShaderModule will be cleaned up in CompileShader by RAII.
			ShaderMetadata Meta(CompileInfo.Filename, CompileInfo.Entrypoint, CompileInfo.Stage, CompileInfo.Type);

			const ShaderCompilationInfo NewCompilationInfo = CompileShader(CompileInfo.Worker, Meta);

			Shader->CompilationInfo = NewCompilationInfo;

			// Destroy cached pipelines with this shader.
			Device.DestroyPipelinesWithShader(Shader);
		}
	}
}