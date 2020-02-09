#include "UserInterface.h"
#include <Engine/Screen.h>
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>

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

	// HACK!
	std::vector<VertexAttributeDescription>& Descriptions = PSODesc.ShaderStages.Vertex->CompilationInfo.VertexAttributeDescriptions;
	Descriptions[2].Format = EFormat::R8G8B8A8_UNORM;
	Screen.ScreenResizeEvent([&] (int32 Width, int32 Height)
	{
		ImGui.DisplaySize = ImVec2((float)Width, (float)Height);

		PSODesc.Viewport.Width = Width;
		PSODesc.Viewport.Height = Height;
	});

	/*attribute_desc[0].location = 0;
	attribute_desc[0].binding = binding_desc[0].binding;
	attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
	attribute_desc[1].location = 1;
	attribute_desc[1].binding = binding_desc[0].binding;
	attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
	attribute_desc[2].location = 2;
	attribute_desc[2].binding = binding_desc[0].binding;
	attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attribute_desc[2].offset = IM_OFFSETOF(ImDrawVert, col);*/
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

		const std::vector<drm::BufferRef> VertexBuffers = { PosBuffer, UvBuffer, ColBuffer };

		CmdList.BindVertexBuffers(VertexBuffers.size(), VertexBuffers.data());

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

				// @todo SetScissor / dynamic state
				PSODesc.Scissor.Offset.x = static_cast<int32_t>(ClipRect.x);
				PSODesc.Scissor.Offset.y = static_cast<int32_t>(ClipRect.y);
				PSODesc.Scissor.Extent.x = static_cast<uint32_t>(ClipRect.z - ClipRect.x);
				PSODesc.Scissor.Extent.y = static_cast<uint32_t>(ClipRect.w - ClipRect.y);

				CmdList.BindPipeline(PSODesc);

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

	PosBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::KeepCPUAccessible, DrawData->TotalVtxCount * sizeof(ImVec2));
	UvBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::KeepCPUAccessible, DrawData->TotalVtxCount * sizeof(ImVec2));
	ColBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::KeepCPUAccessible, DrawData->TotalVtxCount * sizeof(ImU32));

	IndexBuffer = Device.CreateBuffer(EBufferUsage::Index | EBufferUsage::KeepCPUAccessible, IndexBufferSize);

	ImVec2* PosData = static_cast<ImVec2*>(Device.LockBuffer(PosBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		for (int32 i = 0; i < DrawList->VtxBuffer.Size; i++)
		{
			PosData[i] = DrawList->VtxBuffer.Data[i].pos;
		}
		PosData += DrawList->VtxBuffer.Size;
	}

	Device.UnlockBuffer(PosBuffer);

	ImVec2* UvData = static_cast<ImVec2*>(Device.LockBuffer(UvBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		for (int32 i = 0; i < DrawList->VtxBuffer.Size; i++)
		{
			UvData[i] = DrawList->VtxBuffer.Data[i].uv;
		}
		UvData += DrawList->VtxBuffer.Size;
	}

	Device.UnlockBuffer(UvBuffer);

	ImU32* ColData = static_cast<ImU32*>(Device.LockBuffer(ColBuffer));

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		for (int32 i = 0; i < DrawList->VtxBuffer.Size; i++)
		{
			ColData[i] = DrawList->VtxBuffer.Data[i].col;
		}
		ColData += DrawList->VtxBuffer.Size;
	}

	Device.UnlockBuffer(ColBuffer);

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