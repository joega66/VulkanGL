#include "UserInterface.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <Components/RenderSettings.h>
#include <Systems/SceneSystem.h>
#include <Renderer/GlobalRenderData.h>

class UserInterfaceVS : public drm::Shader
{
public:
	UserInterfaceVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Info = { "../Shaders/UserInterfaceVS.glsl", "main", EShaderStage::Vertex };
		return Info;
	}
};

class UserInterfaceFS : public drm::Shader
{
public:
	UserInterfaceFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Info = { "../Shaders/UserInterfaceFS.glsl", "main", EShaderStage::Fragment };
		return Info;
	}
};

UserInterface::~UserInterface()
{
	ImGui::DestroyContext();
}

void UserInterface::Start(Engine& Engine)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(Engine._Platform.Window, true);

	ImGuiIO& ImGui = ImGui::GetIO();
	ImGui.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	ImGui::StyleColorsDark();

	ImGuiRenderData& RenderData = Engine.ECS.AddSingletonComponent<ImGuiRenderData>(Engine.Device);

	auto& GlobalData = Engine.ECS.GetSingletonComponent<GlobalRenderData>();

	PipelineStateDesc PSODesc;
	PSODesc.ColorBlendAttachmentStates.resize(1, {});
	PSODesc.ColorBlendAttachmentStates[0].BlendEnable = true;
	PSODesc.ColorBlendAttachmentStates[0].SrcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].ColorBlendOp = EBlendOp::ADD;
	PSODesc.ColorBlendAttachmentStates[0].SrcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstAlphaBlendFactor = EBlendFactor::ZERO;
	PSODesc.ColorBlendAttachmentStates[0].AlphaBlendOp = EBlendOp::ADD;
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSODesc.ShaderStages.Vertex = Engine.ShaderMap.FindShader<UserInterfaceVS>();
	PSODesc.ShaderStages.Fragment = Engine.ShaderMap.FindShader<UserInterfaceFS>();
	PSODesc.DynamicStates.push_back(EDynamicState::Scissor);
	PSODesc.VertexAttributes = {
		{ 0, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
		{ 1, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
		{ 2, 0, EFormat::R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };
	PSODesc.VertexBindings = { { 0, sizeof(ImDrawVert) } };
	PSODesc.DescriptorSets = { &RenderData.DescriptorSet };
	PSODesc.RenderPass = GlobalData.LightingRP;

	Engine._Screen.ScreenResizeEvent([&Engine, &ImGui, &RenderData, PSODesc] (int32 Width, int32 Height) mutable
	{
		ImGui.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
		PSODesc.Viewport.Width = Width;
		PSODesc.Viewport.Height = Height;
		RenderData.Pipeline = Engine.Device.CreatePipeline(PSODesc);
	});
}

void UserInterface::Update(Engine& Engine)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ShowUI(Engine);

	ImGui::ShowDemoWindow();

	ImGui::Render();

	Engine.ECS.GetSingletonComponent<ImGuiRenderData>().Update(Engine.Device);
}

void UserInterface::ShowUI(Engine& Engine)
{
	ShowRenderSettings(Engine);
	ShowMainMenu(Engine);
}

void UserInterface::ShowMainMenu(Engine& Engine)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				if (const std::filesystem::path FilePath = Engine._Platform.DisplayFileExplorer(); !FilePath.empty())
				{
					auto Message = Engine.ECS.CreateEntity();
					Engine.ECS.AddComponent(Message, SceneLoadRequest{ FilePath, true });
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void UserInterface::ShowRenderSettings(Engine& Engine)
{
	auto& ECS = Engine.ECS;
	
	if (!ImGui::Begin("Render Settings"))
	{
		ImGui::End();
		return;
	}

	auto& RenderSettings = ECS.GetSingletonComponent<class RenderSettings>();

	ImGui::Checkbox("DrawVoxels", &RenderSettings.DrawVoxels);

	ImGui::End();
}

ImGuiRenderData::ImGuiRenderData(DRMDevice& Device)
{
	ImGuiIO& Imgui = ImGui::GetIO();

	unsigned char* Pixels;
	int32 Width, Height;
	Imgui.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

	FontImage = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	ImguiUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(glm::mat4));

	struct ImGuiDescriptors
	{
		drm::ImageView FontImage;
		drm::BufferView ImguiUniform;

		static const std::vector<DescriptorBinding>& GetBindings()
		{
			static const std::vector<DescriptorBinding> Bindings =
			{
				{ 0, 1, EDescriptorType::SampledImage },
				{ 1, 1, EDescriptorType::UniformBuffer }
			};
			return Bindings;
		}
	};

	DescriptorSetLayout = Device.CreateDescriptorSetLayout(ImGuiDescriptors::GetBindings().size(), ImGuiDescriptors::GetBindings().data());
	DescriptorSet = DescriptorSetLayout.CreateDescriptorSet();

	ImGuiDescriptors Descriptors;
	Descriptors.FontImage = drm::ImageView(FontImage, Device.CreateSampler({}));
	Descriptors.ImguiUniform = ImguiUniform;

	DescriptorSetLayout.UpdateDescriptorSet(DescriptorSet, &Descriptors);

	drm::UploadImageData(Device, Pixels, FontImage);

	Imgui.Fonts->TexID = (ImTextureID)(intptr_t)FontImage.GetNativeHandle();
}

void ImGuiRenderData::Render(drm::CommandList& CmdList)
{
	const ImDrawData* DrawData = ImGui::GetDrawData();

	if (DrawData->CmdListsCount > 0)
	{
		CmdList.BindPipeline(Pipeline);

		std::vector<const drm::DescriptorSet*> DescriptorSets = { &DescriptorSet };
		CmdList.BindDescriptorSets(Pipeline, DescriptorSets.size(), DescriptorSets.data());

		CmdList.BindVertexBuffers(1, &VertexBuffer);

		const ImVec2 ClipOff = DrawData->DisplayPos;         // (0,0) unless using multi-viewports
		const ImVec2 ClipScale = DrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		int32 VertexOffset = 0;
		int32 IndexOffset = 0;

		for (int32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
		{
			const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndex];

			for (int32 DrawCmdIndex = 0; DrawCmdIndex < DrawList->CmdBuffer.Size; DrawCmdIndex++)
			{
				const ImDrawCmd* DrawCmd = &DrawList->CmdBuffer[DrawCmdIndex];

				ImVec4 ClipRect;
				ClipRect.x = std::max((DrawCmd->ClipRect.x - ClipOff.x) * ClipScale.x, 0.0f);
				ClipRect.y = std::max((DrawCmd->ClipRect.y - ClipOff.y) * ClipScale.y, 0.0f);
				ClipRect.z = (DrawCmd->ClipRect.z - ClipOff.x) * ClipScale.x;
				ClipRect.w = (DrawCmd->ClipRect.w - ClipOff.y) * ClipScale.y;

				ScissorDesc Scissor;
				Scissor.Offset.x = static_cast<int32_t>(ClipRect.x);
				Scissor.Offset.y = static_cast<int32_t>(ClipRect.y);
				Scissor.Extent.x = static_cast<uint32_t>(ClipRect.z - ClipRect.x);
				Scissor.Extent.y = static_cast<uint32_t>(ClipRect.w - ClipRect.y);

				CmdList.SetScissor(1, &Scissor);

				CmdList.DrawIndexed(IndexBuffer, DrawCmd->ElemCount, 1, DrawCmd->IdxOffset + IndexOffset, DrawCmd->VtxOffset + VertexOffset, 0, EIndexType::UINT16);
			}

			IndexOffset += DrawList->IdxBuffer.Size;
			VertexOffset += DrawList->VtxBuffer.Size;
		}
	}
}

void ImGuiRenderData::Update(DRMDevice& Device)
{
	const ImDrawData* DrawData = ImGui::GetDrawData();
	const uint32 VertexBufferSize = DrawData->TotalVtxCount * sizeof(ImDrawVert);
	const uint32 IndexBufferSize = DrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((VertexBufferSize == 0) || (IndexBufferSize == 0))
	{
		return;
	}

	// Upload ImGui vertex/index buffer data.

	VertexBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::HostVisible, VertexBufferSize);
	ImDrawVert* VertexData = static_cast<ImDrawVert*>(Device.LockBuffer(VertexBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		Platform::Memcpy(VertexData, DrawList->VtxBuffer.Data, DrawList->VtxBuffer.Size * sizeof(ImDrawVert));
		VertexData += DrawList->VtxBuffer.Size;
	}

	Device.UnlockBuffer(VertexBuffer);

	IndexBuffer = Device.CreateBuffer(EBufferUsage::Index | EBufferUsage::HostVisible, IndexBufferSize);
	ImDrawIdx* IndexData = static_cast<ImDrawIdx*>(Device.LockBuffer(IndexBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		Platform::Memcpy(IndexData, DrawList->IdxBuffer.Data, DrawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		IndexData += DrawList->IdxBuffer.Size;
	}

	Device.UnlockBuffer(IndexBuffer);

	ImGuiIO& ImGui = ImGui::GetIO();
	glm::vec4* ImGuiData = static_cast<glm::vec4*>(Device.LockBuffer(ImguiUniform));
	ImGuiData->x = 2.0f / ImGui.DisplaySize.x;
	ImGuiData->y = 2.0f / ImGui.DisplaySize.y;
	ImGuiData->z = -1.0f - DrawData->DisplayPos.x * ImGuiData->x;
	ImGuiData->w = -1.0f - DrawData->DisplayPos.y * ImGuiData->y;
	Device.UnlockBuffer(ImguiUniform);
}