#include "UserInterface.h"
#include <Engine/Screen.h>
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <DRMShader.h>

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

UserInterface::UserInterface(Platform& Platform, DRMDevice& Device, DRMShaderMap& ShaderMap, Screen& Screen)
	: Descriptors(Device)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(Platform.Window, true);

	ImGuiIO& ImGui = ImGui::GetIO();
	ImGui.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	ImGui::StyleColorsDark();

	CreateImGuiRenderResources(Device);

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
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<UserInterfaceVS>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<UserInterfaceFS>();
	PSODesc.DynamicStates.push_back(EDynamicState::Scissor);
	PSODesc.VertexAttributes = {
		{ 0, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, pos) }, 
		{ 1, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
		{ 2, 0, EFormat::R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };
	PSODesc.VertexBindings = { { 0, sizeof(ImDrawVert) } };

	Screen.ScreenResizeEvent([&] (int32 Width, int32 Height)
	{
		ImGui.DisplaySize = ImVec2((float)Width, (float)Height);

		PSODesc.Viewport.Width = Width;
		PSODesc.Viewport.Height = Height;
	});
}

UserInterface::~UserInterface()
{
	ImGui::DestroyContext();
}

void UserInterface::Start(EntityManager& ECS, DRMDevice& Device)
{
}

void UserInterface::Update(EntityManager& ECS, DRMDevice& Device)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
	
	ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
	ImGui::End();
	
	ImGui::Render();

	UploadImGuiDrawData(Device);
}

void UserInterface::Render(drm::CommandList& CmdList)
{
	const ImDrawData* DrawData = ImGui::GetDrawData();

	if (DrawData->CmdListsCount > 0)
	{
		drm::DescriptorSetRef DescriptorSet = Descriptors;

		CmdList.BindDescriptorSets(1, &DescriptorSet);

		CmdList.BindPipeline(PSODesc);

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

void UserInterface::CreateImGuiRenderResources(DRMDevice& Device)
{
	ImGuiIO& Imgui = ImGui::GetIO();

	unsigned char* Pixels;
	int32 Width, Height;
	Imgui.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

	Descriptors.ImguiUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::KeepCPUAccessible, sizeof(glm::mat4));
	Descriptors.FontImage = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Descriptors.Update();

	drm::UploadImageData(Device, Pixels, Descriptors.FontImage);
	
	Imgui.Fonts->TexID = (ImTextureID)(intptr_t)Descriptors.FontImage->GetNativeHandle();
}

void UserInterface::UploadImGuiDrawData(DRMDevice& Device)
{
	const ImDrawData* DrawData = ImGui::GetDrawData();
	const uint32 VertexBufferSize = DrawData->TotalVtxCount * sizeof(ImDrawVert);
	const uint32 IndexBufferSize = DrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((VertexBufferSize == 0) || (IndexBufferSize == 0))
	{
		return;
	}

	// Upload ImGui vertex/index buffer data.

	VertexBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::KeepCPUAccessible, VertexBufferSize);
	ImDrawVert* VertexData = static_cast<ImDrawVert*>(Device.LockBuffer(VertexBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		Platform::Memcpy(VertexData, DrawList->VtxBuffer.Data, DrawList->VtxBuffer.Size * sizeof(ImDrawVert));
		VertexData += DrawList->VtxBuffer.Size;
	}

	Device.UnlockBuffer(VertexBuffer);

	IndexBuffer = Device.CreateBuffer(EBufferUsage::Index | EBufferUsage::KeepCPUAccessible, IndexBufferSize);
	ImDrawIdx* IndexData = static_cast<ImDrawIdx*>(Device.LockBuffer(IndexBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		Platform::Memcpy(IndexData, DrawList->IdxBuffer.Data, DrawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		IndexData += DrawList->IdxBuffer.Size;
	}

	Device.UnlockBuffer(IndexBuffer);

	ImGuiIO& ImGui = ImGui::GetIO();
	glm::vec4* ImGuiData = static_cast<glm::vec4*>(Device.LockBuffer(Descriptors.ImguiUniform));
	ImGuiData->x = 2.0f / ImGui.DisplaySize.x;
	ImGuiData->y = 2.0f / ImGui.DisplaySize.y;
	ImGuiData->z = -1.0f - DrawData->DisplayPos.x * ImGuiData->x;
	ImGuiData->w = -1.0f - DrawData->DisplayPos.y * ImGuiData->y;
	Device.UnlockBuffer(Descriptors.ImguiUniform);
}