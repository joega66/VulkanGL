#include "VulkanShaderMap.h"
#include "VulkanDevice.h"
#include <SPIRV-Cross/spirv_glsl.hpp>

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

		Description.Binding = Descriptions.size();
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

	return Descriptions;
}

VulkanShaderMap::VulkanShaderMap(VulkanDevice& Device)
	: Device(Device)
{
}

ShaderCompilationInfo VulkanShaderMap::CompileShader(
	const ShaderCompilerWorker& Worker,
	const std::string& Filename,
	const std::string& EntryPoint,
	EShaderStage Stage,
	std::type_index Type
)
{
	static const std::string ShaderCompilerPath = "../Shaders/glslc.exe";
	static const std::string SPIRVExt = ".spv";

	ShaderCompilerWorker PrivateWorker;

	const std::string ShaderExt = [&] ()
	{
		switch (Stage)
		{
		case EShaderStage::Vertex:
			PrivateWorker.SetDefine("VERTEX_SHADER");
			return "vert";
		case EShaderStage::TessControl:
			PrivateWorker.SetDefine("TESSCONTROL_SHADER");
			return "tesc";
		case EShaderStage::TessEvaluation:
			PrivateWorker.SetDefine("TESSEVAL_SHADER");
			return "tese";
		case EShaderStage::Geometry:
			PrivateWorker.SetDefine("GEOMETRY_SHADER");
			return "geom";
		case EShaderStage::Fragment:
			PrivateWorker.SetDefine("FRAGMENT_SHADER");
			return "frag";
		default: // Compute
			return "comp";
		}
	}();

	std::stringstream SS;

	const auto SetDefines = [&] (const std::pair<std::string, std::string>& Defines)
	{
		const auto& [Define, Value] = Defines;
		SS << " -D" + Define + "=" + Value;
	};

	std::for_each(Worker.GetDefines().begin(), Worker.GetDefines().end(), SetDefines);

	std::for_each(PrivateWorker.GetDefines().begin(), PrivateWorker.GetDefines().end(), SetDefines);

	SS << " -std=450";
	SS << " -fshader-stage=" + ShaderExt;
	SS << " -o ";
	SS << Filename + SPIRVExt;
	SS << " " + Filename;

	while (true)
	{
		Platform::ForkProcess(ShaderCompilerPath, SS.str());

		// Hack until ForkProcess can return STDOUT of child process.
		if (Platform::FileExists(Filename + SPIRVExt))
		{
			break;
		}

		const EMBReturn Ret = Platform::DisplayMessageBox(EMBType::RETRYCANCEL, EMBIcon::WARNING, "Shader failed to compile. Filename: " + Filename, "Shader Compiler Error");

		switch (Ret)
		{
		case EMBReturn::CANCEL:
			Platform::Exit();
		case EMBReturn::RETRY:
			LOG("Recompiling shader %s", Filename.c_str());
		}
	}

	const uint64 LastWriteTime = Platform::GetLastWriteTime(Filename);

	const std::string SPIRV = Platform::FileRead(Filename + SPIRVExt);
	Platform::FileDelete(Filename + SPIRVExt);

	VkShaderModuleCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	CreateInfo.codeSize = SPIRV.size();
	CreateInfo.pCode = reinterpret_cast<const uint32*>(SPIRV.data());

	VkShaderModule ShaderModule;
	vulkan(vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule));

	spirv_cross::CompilerGLSL GLSL(reinterpret_cast<const uint32*>(SPIRV.data()), SPIRV.size() / sizeof(uint32));
	spirv_cross::ShaderResources Resources = GLSL.get_shader_resources();

	const std::vector<VertexAttributeDescription> VertexAttributeDescriptions = 
		Stage == EShaderStage::Vertex ? ParseVertexAttributeDescriptions(GLSL, Resources) : std::vector<VertexAttributeDescription>{};

	return ShaderCompilationInfo(Type, Stage, EntryPoint, Filename, LastWriteTime, Worker, ShaderModule, VertexAttributeDescriptions);
}

void VulkanShaderMap::RecompileShaders()
{
	for (const auto& [ShaderType, Shader] : Shaders)
	{
		const ShaderCompilationInfo& CompileInfo = Shader->CompilationInfo;

		const uint64 LastWriteTime = Platform::GetLastWriteTime(CompileInfo.Filename);

		if (LastWriteTime > CompileInfo.LastWriteTime)
		{
			LOG("Recompiling shader %s", CompileInfo.Filename.c_str());

			// Destroy the old shader module.
			vkDestroyShaderModule(Device, CompileInfo.Module, nullptr);

			const ShaderCompilationInfo NewCompilationInfo = CompileShader(
				CompileInfo.Worker,
				CompileInfo.Filename, 
				CompileInfo.Entrypoint, 
				CompileInfo.Stage, 
				CompileInfo.Type
			);

			Shader->CompilationInfo = NewCompilationInfo;

			// Destroy cached pipelines with this shader.
			Device.GetCache().DestroyPipelinesWithShader(Shader);
		}
	}
}