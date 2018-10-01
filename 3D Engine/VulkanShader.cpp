#include "VulkanShader.h"
#include "VulkanGL.h"
#include "GL.h"
#include <regex>

struct VertexStreamFormat
{
	int32 Location = -1;
	std::string Type;
	std::string Name;
};

struct UniformBufferFormat
{
	int32 Binding = -1;
	std::string Name;
	uint32 Size = 0;
};

struct TextureFormat
{
	int32 Binding = -1;
	std::string Name;
	std::string Type;
};

const std::string AnyDigit = "(\\d+)";
const std::string GLSLType = "((?:[a-z][a-z]*[0-9]+[a-z0-9]*))";
const std::string GLSLName = "((?:[a-zA-Z0-9_]*))";

const std::string GLSLTextureType("((?:sampler1D|samplerCube|samplerCubeShadow|sampler2DShadow|sampler2D|usampler2D|isampler2D))");
const std::regex GLSLTypesRegex("((?:bool|int|uint|float|vec2|vec3|vec4|ivec2|ivec3|ivec4|uvec2|uvec3|uvec4|mat2|mat3|mat4))");

const std::regex VertexStreamRegex("layout[(]location = " + AnyDigit + "[)] in " + GLSLType + " " + GLSLName + ";");
const std::regex UniformRegex("layout[(]binding = " + AnyDigit + "[)] uniform " + GLSLName + "[{](.*?)" + "[}] " + GLSLName);
const std::regex TextureRegex("layout[(]binding = " + AnyDigit + "[)] uniform " + GLSLTextureType + " " + GLSLName + ";");

const VkShaderStageFlagBits VulkanStages[] =
{
	VK_SHADER_STAGE_VERTEX_BIT,
	VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	VK_SHADER_STAGE_GEOMETRY_BIT,
	VK_SHADER_STAGE_FRAGMENT_BIT,
	VK_SHADER_STAGE_COMPUTE_BIT
};

static std::vector<VertexStreamFormat> ParseVertexStreams(const std::string& Code)
{
	std::vector<VertexStreamFormat> VertexStreams;
	std::smatch Match;
	std::string::const_iterator Iter(Code.cbegin());

	while (std::regex_search(Iter, Code.cend(), Match, VertexStreamRegex))
	{
		VertexStreamFormat Stream;
		Stream.Location = std::stoi(Match[1].str());
		Stream.Type = Match[2].str();
		Stream.Name = Match[3].str();
		VertexStreams.push_back(Stream);
		Iter += Match.position() + Match.length();
	}

	return VertexStreams;
}

static std::vector<UniformBufferFormat> ParseUniforms(const std::string& Code)
{
	std::vector<UniformBufferFormat> Uniforms;
	std::string CodeCopy = Code;
	GPlatform->RemoveNewlines(CodeCopy);
	std::smatch Match;
	std::string::const_iterator Iter(CodeCopy.cbegin());

	while (std::regex_search(Iter, CodeCopy.cend(), Match, UniformRegex))
	{
		UniformBufferFormat Uniform = {};;
		Uniform.Binding = std::stoi(Match[1].str());
		Uniform.Name = Match[4].str();

		std::string Members = Match[3].str();
		IPlatform::RemoveSpaces(Members);
		std::string::const_iterator InnerIter(Members.cbegin());
		std::smatch InnerMatch;
		std::regex_search(InnerIter, Members.cend(), InnerMatch, GLSLTypesRegex);

		for (const auto& Member : InnerMatch)
		{
			Uniform.Size += GetValue(GLSLTypeSizes, Member.str());
		}

		check(Uniform.Size % 16 == 0, "std140 layout advises to manually pad structures/arrays to multiple of 16 bytes.");

		Uniforms.push_back(Uniform);
		Iter += Match.position() + Match.length();
	}

	return Uniforms;
}

static std::vector<TextureFormat> ParseTextures(const std::string& Code)
{
	std::vector<TextureFormat> Textures;
	std::smatch Match;
	std::string::const_iterator Iter(Code.cbegin());

	while (std::regex_search(Iter, Code.cend(), Match, TextureRegex))
	{
		TextureFormat Texture;
		Texture.Binding = std::stoi(Match[1].str());
		Texture.Type = Match[2].str();
		Texture.Name = Match[3].str();
		Textures.push_back(Texture);
		Iter += Match.position() + Match.length();
	}

	return Textures;
}

static std::vector<VkVertexInputAttributeDescription> CreateVertexInputAttributeDescriptions(const std::vector<VertexStreamFormat>& Streams)
{
	std::vector<VkVertexInputAttributeDescription> Descriptions;

	for (const auto& Stream : Streams)
	{
		VkVertexInputAttributeDescription Description = {};
		Description.binding = 0;
		Description.location = Stream.Location;
		Description.format = GetValue(GLSLTypeToVulkanFormat, Stream.Type);
		Description.offset = 0;
		Descriptions.push_back(Description);
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

GLShaderRef VulkanShaderCompiler::CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta)
{
	static const std::string ShaderCompilerPath = "C:/VulkanSDK/1.1.73.0/Bin32/glslangValidator.exe";
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
	SS << " -S " + ShaderExt;
	SS << " -e " + Meta.EntryPoint;
	SS << " -o ";
	SS << Meta.Filename + SPIRVExt;
	SS << " -V ";
	SS << Meta.Filename;

	GPlatform->ForkProcess(ShaderCompilerPath, SS.str());

	// Hack until ForkProcess can return STDOUT of child process. 
	check(GPlatform->FileExists(Meta.Filename + SPIRVExt),
		"Shader failed to compile.\nFilename: %s", Meta.Filename.c_str());
	const std::string SPIRV = GPlatform->FileRead(Meta.Filename + SPIRVExt);
	GPlatform->FileDelete(Meta.Filename + SPIRVExt);

	VulkanGLRef Vulkan = std::static_pointer_cast<VulkanGL>(GRender);

	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = SPIRV.size();
	CreateInfo.pCode = reinterpret_cast<const uint32*>(SPIRV.data());

	VkShaderModule ShaderModule;
	vulkan(vkCreateShaderModule(*Vulkan, &CreateInfo, nullptr, &ShaderModule));

	// Parse vertex attributes and shader resources.

	const std::string& Code = Worker.Code;
	VkShaderStageFlags Stage = VulkanStages[(int32)Meta.Stage];
	std::vector<VertexStreamFormat> VertexStreams = ParseVertexStreams(Code);
	std::vector<UniformBufferFormat> Uniforms = ParseUniforms(Code);
	std::vector<TextureFormat> Textures = ParseTextures(Code);
	std::vector<VkVertexInputAttributeDescription> Attributes = CreateVertexInputAttributeDescriptions(VertexStreams);
	std::vector<VkDescriptorSetLayoutBinding> Bindings;

	CreateDescriptorSetLayoutBindings(Uniforms, Bindings, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Stage);
	CreateDescriptorSetLayoutBindings(Textures, Bindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Stage);
	// @todo-joe UAVs

	std::sort(Attributes.begin(), Attributes.end(),
		[] (const VkVertexInputAttributeDescription& LHS, const VkVertexInputAttributeDescription& RHS)
	{
		return LHS.location < RHS.location;
	});

	std::sort(Bindings.begin(), Bindings.end(),
		[] (const VkDescriptorSetLayoutBinding& LHS, const VkDescriptorSetLayoutBinding& RHS)
	{
		return LHS.binding < RHS.binding;
	});

	return MakeRef<VulkanShader>(Vulkan->GetDevice(), ShaderModule, Meta, Attributes, Bindings);
}

VulkanShader::VulkanShader(
	VulkanDevice& Device
	, VkShaderModule ShaderModule
	, const ShaderMetadata& Meta
	, const std::vector<VkVertexInputAttributeDescription>& Attributes
	, const std::vector<VkDescriptorSetLayoutBinding>& Bindings
) : Device(Device), ShaderModule(ShaderModule), Attributes(Attributes), Bindings(Bindings), GLShader(Meta)
{
}

void VulkanShader::ReleaseGL()
{
	vkDestroyShaderModule(Device, ShaderModule, nullptr);
}

VkShaderStageFlagBits VulkanShader::GetVulkanStage() const
{
	return VulkanStages[(int32)Meta.Stage];
}