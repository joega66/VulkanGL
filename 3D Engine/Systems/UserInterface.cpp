#include "UserInterface.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <Components/RenderSettings.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Bounds.h>
#include <Components/SkyboxComponent.h>
#include <Systems/SceneSystem.h>
#include <Renderer/Voxels.h>
#include <Renderer/CameraProxy.h>
#include <Renderer/ShadowProxy.h>

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

	ImGuiIO& IO = ImGui::GetIO();
	IO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	ImGuiStyle& Style = ImGui::GetStyle();
	Style.FrameBorderSize = 1.0f;

	ImGui::StyleColorsDark();

	Engine.ECS.AddSingletonComponent<ImGuiRenderData>(Engine);
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
	ShowEntities(Engine);
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

	auto& Settings = ECS.GetSingletonComponent<RenderSettings>();
	int& VoxelDebugMode = reinterpret_cast<int&>(Settings.VoxelDebugMode);

	if (ImGui::TreeNode("Voxel Cone Tracing"))
	{
		ImGui::Checkbox("Voxelize", &Settings.bVoxelize);

		if (ImGui::DragFloat("Size", &Settings.VoxelSize))
		{
			Settings.bVoxelize = true;
		}

		if (ImGui::DragFloat3("Position", &Settings.VoxelFieldCenter[0]))
		{
			Settings.bVoxelize = true;
		}

		ImGui::RadioButton("None", &VoxelDebugMode, (int)EVoxelDebugMode::None);
		ImGui::SameLine(); ImGui::RadioButton("Voxel Radiance", &VoxelDebugMode, (int)EVoxelDebugMode::Radiance);
		ImGui::SameLine(); ImGui::RadioButton("Voxel Base Color", &VoxelDebugMode, (int)EVoxelDebugMode::BaseColor);
		ImGui::SameLine(); ImGui::RadioButton("Voxel Normal", &VoxelDebugMode, (int)EVoxelDebugMode::Normal);
		ImGui::TreePop();
	}
	
	if (ImGui::TreeNode("Camera"))
	{
		ImGui::DragFloat("Exposure Adjustment", &Settings.ExposureAdjustment, 0.05f, 0.0f, 256.0f);
		ImGui::DragFloat("Exposure Bias", &Settings.ExposureBias, 0.05f, 0.0f, 16.0f);
		ImGui::TreePop();
	}

	ImGui::End();
}

void UserInterface::ShowEntities(Engine& Engine)
{
	if (!ImGui::Begin("Entities"))
	{
		ImGui::End();
		return;
	}

	EntityManager& ECS = Engine.ECS;
	EntityIterator EntityIter = ECS.Iter();
	static Entity Selected;
	static ImGuiTextFilter Filter;

	Filter.Draw("");

	while (!EntityIter.End())
	{
		auto& Entity = EntityIter.Next();

		const auto& Name = ECS.GetName(Entity);

		if (Filter.PassFilter(Name.c_str()))
		{
			bool IsSelected = ImGui::Selectable(Name.c_str(), Selected == Entity, ImGuiSelectableFlags_AllowDoubleClick);

			if (IsSelected)
			{
				Selected = Entity;
			}
			
			if (IsSelected && ImGui::IsMouseDoubleClicked(0) && ECS.HasComponent<Bounds>(Entity))
			{
				const auto& Bounds = ECS.GetComponent<class Bounds>(Entity);
				const glm::vec3 Center = Bounds.Box.GetCenter();
				Engine.Camera.LookAt(Center);
			}
		}
	}
	
	ImGui::End();

	if (!ImGui::Begin("Components") || !ECS.IsValid(Selected))
	{
		ImGui::End();
		return;
	}

	if (ECS.HasComponent<Transform>(Selected))
	{
		ImGui::Text("Transform");

		auto& Transform = ECS.GetComponent<class Transform>(Selected);
		glm::vec3 Position = Transform.GetPosition();
		glm::vec3 Rotation = Transform.GetRotation();
		glm::vec3 Scale = Transform.GetScale();
		float Angle = Transform.GetAngle();

		ImGui::DragFloat3("Position", &Position[0], 0.05f);
		ImGui::DragFloat3("Rotation", &Rotation[0], 0.05f);
		ImGui::DragFloat("Angle", &Angle, 1.0f, -180.0, 180.0f);
		ImGui::DragFloat3("Scale", &Scale[0], 0.05f);

		if (Position != Transform.GetPosition() ||
			Rotation != Transform.GetRotation() ||
			Scale != Transform.GetScale() || 
			Angle != Transform.GetAngle())
		{
			Transform.Translate(ECS, Position);
			Transform.Rotate(ECS, Rotation, Angle);
			Transform.Scale(ECS, Scale);

			auto& Settings = ECS.GetSingletonComponent<RenderSettings>();
			Settings.bVoxelize = true;
		}
	}

	if (ECS.HasComponent<DirectionalLight>(Selected))
	{
		ImGui::Text("Directional Light");

		auto& Light = ECS.GetComponent<DirectionalLight>(Selected);
		glm::vec3 Direction = Light.Direction;
		glm::vec3 Color = Light.Color;
		float Intensity = Light.Intensity;

		ImGui::DragFloat3("Direction", &Direction[0]);
		ImGui::ColorEdit3("Color", &Color[0]);
		ImGui::DragFloat("Intensity", &Intensity, 1.0f, 0.0f);

		if (Direction != Light.Direction ||
			Color != Light.Color ||
			Intensity != Light.Intensity)
		{
			Light.Direction = Direction;
			Light.Color = Color;
			Light.Intensity = Intensity;

			auto& Settings = ECS.GetSingletonComponent<RenderSettings>();
			Settings.bVoxelize = true;
		}

		if (ECS.HasComponent<ShadowProxy>(Selected))
		{
			ImGui::Text("Shadows");
			auto& Shadow = ECS.GetComponent<ShadowProxy>(Selected);
			ImGui::DragFloat("Width", &Shadow.Width);
			ImGui::DragFloat("ZNear", &Shadow.ZNear);
			ImGui::DragFloat("ZFar", &Shadow.ZFar);
		}
	}

	if (ECS.HasComponent<SkyboxComponent>(Selected))
	{
		ImGui::Text("Skybox");

		auto& Device = Engine.Device;
		auto& SkyboxComp = ECS.GetComponent<SkyboxComponent>(Selected);
		Skybox* Skybox = SkyboxComp.Skybox;
		
		for (uint32 Face = CubemapFace_Begin; Face != CubemapFace_End; Face++)
		{
			drm::TextureID& TextureID = const_cast<drm::TextureID&>(const_cast<drm::Image*>(Skybox->GetFaces()[Face])->GetTextureID());
			ImGui::Text(Skybox::CubemapFaces[Face].c_str());
			ImGui::SameLine();
			ImGui::ImageButton(
				&TextureID,
				ImVec2(64.0f, 64.0f)
			);
		}
	}

	ImGui::End();
}

ImGuiRenderData::ImGuiRenderData(Engine& Engine)
{
	DRMDevice& Device = Engine.Device;
	ImGuiIO& Imgui = ImGui::GetIO();

	unsigned char* Pixels;
	int32 Width, Height;
	Imgui.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

	FontImage = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	SamplerID = Device.CreateSampler({}).GetSamplerID();

	drm::UploadImageData(Device, Pixels, FontImage);

	Imgui.Fonts->TexID = &const_cast<drm::TextureID&>(FontImage.GetTextureID());

	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSODesc.ShaderStages.Vertex = Engine.ShaderLibrary.FindShader<UserInterfaceVS>();
	PSODesc.ShaderStages.Fragment = Engine.ShaderLibrary.FindShader<UserInterfaceFS>();
	PSODesc.ColorBlendAttachmentStates.resize(1, {});
	PSODesc.ColorBlendAttachmentStates[0].BlendEnable = true;
	PSODesc.ColorBlendAttachmentStates[0].SrcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].ColorBlendOp = EBlendOp::ADD;
	PSODesc.ColorBlendAttachmentStates[0].SrcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstAlphaBlendFactor = EBlendFactor::ZERO;
	PSODesc.ColorBlendAttachmentStates[0].AlphaBlendOp = EBlendOp::ADD;
	PSODesc.DynamicStates.push_back(EDynamicState::Scissor);
	PSODesc.VertexAttributes = {
		{ 0, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
		{ 1, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
		{ 2, 0, EFormat::R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };
	PSODesc.VertexBindings = { { 0, sizeof(ImDrawVert) } };
	PSODesc.Layouts = { Device.GetTextures().GetLayout(), Device.GetSamplers().GetLayout() };
	PSODesc.PushConstantRanges.push_back({ EShaderStage::Vertex, 0, sizeof(ScaleAndTranslation) });
	PSODesc.PushConstantRanges.push_back({ EShaderStage::Fragment, sizeof(ScaleAndTranslation), sizeof(glm::uvec2) });

	Engine._Screen.ScreenResizeEvent([this, &Device] (int32 Width, int32 Height)
	{
		ImGuiIO& ImGui = ImGui::GetIO();
		ImGui.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));

		PSODesc.Viewport.Width = Width;
		PSODesc.Viewport.Height = Height;
	});
}

void ImGuiRenderData::Render(DRMDevice& Device, drm::CommandList& CmdList, const drm::RenderPass& RenderPass)
{
	CmdList.BeginRenderPass(RenderPass);

	const ImDrawData* DrawData = ImGui::GetDrawData();

	if (DrawData->CmdListsCount > 0)
	{
		PSODesc.RenderPass = RenderPass;

		drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

		CmdList.BindPipeline(Pipeline);

		CmdList.PushConstants(Pipeline, EShaderStage::Vertex, 0, sizeof(ScaleAndTranslation), &ScaleAndTranslation);

		const std::vector<VkDescriptorSet> DescriptorSets = { Device.GetTextures().GetSet(), Device.GetSamplers().GetSet() };
		CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

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

				const glm::uvec2 PushConstants(*static_cast<uint32*>(DrawCmd->TextureId), SamplerID);
				CmdList.PushConstants(Pipeline, EShaderStage::Fragment, sizeof(ScaleAndTranslation), sizeof(PushConstants), &PushConstants);

				CmdList.DrawIndexed(IndexBuffer, DrawCmd->ElemCount, 1, DrawCmd->IdxOffset + IndexOffset, DrawCmd->VtxOffset + VertexOffset, 0, EIndexType::UINT16);
			}

			IndexOffset += DrawList->IdxBuffer.Size;
			VertexOffset += DrawList->VtxBuffer.Size;
		}
	}

	CmdList.EndRenderPass();
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

	VertexBuffer = Device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::HostVisible, VertexBufferSize);
	ImDrawVert* VertexData = static_cast<ImDrawVert*>(VertexBuffer.GetData());
	IndexBuffer = Device.CreateBuffer(EBufferUsage::Index | EBufferUsage::HostVisible, IndexBufferSize);
	ImDrawIdx* IndexData = static_cast<ImDrawIdx*>(IndexBuffer.GetData());

	for (int32 CmdListIndx = 0; CmdListIndx < DrawData->CmdListsCount; CmdListIndx++)
	{
		const ImDrawList* DrawList = DrawData->CmdLists[CmdListIndx];
		Platform::Memcpy(VertexData, DrawList->VtxBuffer.Data, DrawList->VtxBuffer.Size * sizeof(ImDrawVert));
		Platform::Memcpy(IndexData, DrawList->IdxBuffer.Data, DrawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		VertexData += DrawList->VtxBuffer.Size;
		IndexData += DrawList->IdxBuffer.Size;
	}

	ImGuiIO& ImGui = ImGui::GetIO();
	
	ScaleAndTranslation.x = 2.0f / ImGui.DisplaySize.x;
	ScaleAndTranslation.y = 2.0f / ImGui.DisplaySize.y;
	ScaleAndTranslation.z = -1.0f - DrawData->DisplayPos.x * ScaleAndTranslation.x;
	ScaleAndTranslation.w = -1.0f - DrawData->DisplayPos.y * ScaleAndTranslation.y;
}