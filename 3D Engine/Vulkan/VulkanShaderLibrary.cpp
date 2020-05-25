#include "VulkanShaderLibrary.h"
#include "VulkanDevice.h"
#include <SPIRV-Cross/spirv_glsl.hpp>
#include <shaderc/shaderc.hpp>

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

static std::vector<VertexAttributeDescription> ParseVertexAttributeDescriptions(const spirv_cross::CompilerGLSL& GLSL, const spirv_cross::ShaderResources& Resources)
{
	std::vector<VertexAttributeDescription> Descriptions;

	for (auto& Resource : Resources.stage_inputs)
	{
		VertexAttributeDescription Description = {};
		Description.Location = GLSL.get_decoration(Resource.id, spv::DecorationLocation);
		Description.Format = VulkanImage::GetEngineFormat(GetFormatFromBaseType(GLSL.get_type(Resource.type_id)));
		Description.Offset = 0;

		Descriptions.push_back(Description);
	}

	// This sorting saves from having to figure out
	// a mapping between layout(location = ...) and buffer binding point
	std::sort(Descriptions.begin(), Descriptions.end(),
		[] (const VertexAttributeDescription& LHS, const VertexAttributeDescription& RHS)
	{
		return LHS.Location < RHS.Location;
	});

	for (uint32_t Binding = 0; Binding < Descriptions.size(); Binding++)
	{
		Descriptions[Binding].Binding = Binding;
	}

	return Descriptions;
}

class ShadercIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	shaderc_include_result* GetInclude(
		const char* requested_source,
		shaderc_include_type type,
		const char* requesting_source,
		size_t include_depth) override
	{
		static const std::string ShaderPath = "../Shaders/";

		auto& SourceName = SourceNames.emplace_back(ShaderPath + std::string(requested_source));

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

	const std::string ShaderStruct = Worker.GetPushConstantStruct().empty() == false ? 
		"layout(push_constant) uniform _ShaderStruct{" + Worker.GetPushConstantStruct() + "};\n"
		: "";
	
	shaderc::Compiler Compiler;
	shaderc::SpvCompilationResult SpvCompilationResult;

	do
	{
		const std::string SourceText = ShaderStruct + Platform::FileRead(Filename);

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
		ParseVertexAttributeDescriptions(GLSL, Resources) : std::vector<VertexAttributeDescription>{};
	const uint64 LastWriteTime = Platform::GetLastWriteTime(Filename);

	PushConstantRange PushConstantRange;
	PushConstantRange.StageFlags = Stage;
	PushConstantRange.Offset = Worker.GetPushConstantOffset();
	PushConstantRange.Size = Worker.GetPushConstantSize();

	return ShaderCompilationInfo(Type, Stage, EntryPoint, Filename, LastWriteTime, Worker, ShaderModule, VertexAttributeDescriptions, PushConstantRange);
}

void VulkanShaderLibrary::RecompileShaders()
{
	for (const auto& [ShaderType, Shader] : Shaders)
	{
		const ShaderCompilationInfo& CompileInfo = Shader->CompilationInfo;
		const uint64 LastWriteTime = Platform::GetLastWriteTime(CompileInfo.Filename);

		//if (LastWriteTime > CompileInfo.LastWriteTime)
		{
			// Destroy the old shader module.
			vkDestroyShaderModule(Device, static_cast<VkShaderModule>(CompileInfo.Module), nullptr);

			const ShaderCompilationInfo NewCompilationInfo = CompileShader(
				CompileInfo.Worker,
				CompileInfo.Filename, 
				CompileInfo.Entrypoint,
				CompileInfo.Stage, 
				CompileInfo.Type
			);

			Shader->CompilationInfo = NewCompilationInfo;
		}
	}

	Device.GetCache().RecompilePipelines();
}