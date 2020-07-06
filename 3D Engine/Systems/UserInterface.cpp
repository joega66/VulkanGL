#include "UserInterface.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <Engine/Engine.h>
#include <Engine/Screen.h>
#include <Engine/Input.h>
#include <Components/RenderSettings.h>
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Bounds.h>
#include <Components/SkyboxComponent.h>
#include <Systems/SceneSystem.h>
#include <Renderer/CameraProxy.h>
#include <Renderer/ShadowProxy.h>

class UserInterfaceVS : public gpu::Shader
{
public:
	UserInterfaceVS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/UserInterfaceVS.glsl", "main", EShaderStage::Vertex };
		return info;
	}
};

class UserInterfaceFS : public gpu::Shader
{
public:
	UserInterfaceFS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/UserInterfaceFS.glsl", "main", EShaderStage::Fragment };
		return info;
	}
};

UserInterface::~UserInterface()
{
	ImGui::DestroyContext();
}

void UserInterface::Start(Engine& engine)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(engine._Platform.Window, true);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui::StyleColorsDark();

	engine._ECS.AddSingletonComponent<ImGuiRenderData>(engine);
}

void UserInterface::Update(Engine& engine)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ShowUI(engine);

	ImGui::ShowDemoWindow();

	ImGui::Render();

	engine._ECS.GetSingletonComponent<ImGuiRenderData>().Update(engine.Device);
}

void UserInterface::ShowUI(Engine& engine)
{
	ShowRenderSettings(engine);
	ShowMainMenu(engine);
	ShowEntities(engine);
}

void UserInterface::ShowMainMenu(Engine& engine)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				if (const std::filesystem::path filePath = engine._Platform.DisplayFileExplorer(); !filePath.empty())
				{
					auto message = engine._ECS.CreateEntity();
					engine._ECS.AddComponent(message, SceneLoadRequest{ filePath, true });
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void UserInterface::ShowRenderSettings(Engine& engine)
{
	auto& ecs = engine._ECS;
	
	if (!ImGui::Begin("Render Settings"))
	{
		ImGui::End();
		return;
	}

	auto& settings = ecs.GetSingletonComponent<RenderSettings>();

	if (ImGui::TreeNode("Camera"))
	{
		ImGui::DragFloat("Exposure Adjustment", &settings.ExposureAdjustment, 0.05f, 0.0f, 256.0f);
		ImGui::DragFloat("Exposure Bias", &settings.ExposureBias, 0.05f, 0.0f, 16.0f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Ray Tracing"))
	{
		ImGui::Checkbox("Ray Tracing", &settings.bRayTracing);
		ImGui::TreePop();
	}

	ImGui::End();
}

void UserInterface::ShowEntities(Engine& engine)
{
	if (!ImGui::Begin("Entities"))
	{
		ImGui::End();
		return;
	}

	EntityManager& ecs = engine._ECS;
	EntityIterator entityIter = ecs.Iter();

	static Entity entitySelected;
	static Entity entityAfterSelected;
	static ImGuiTextFilter entitySearchBar;	

	if (ImGui::Button("New Entity"))
	{
		LOG("Creating an entity!");
		entitySelected = ecs.CreateEntity();
	}

	ImGui::SameLine();
	entitySearchBar.Draw("");

	Entity prevEntity;

	while (!entityIter.End())
	{
		auto& entity = entityIter.Next();

		const auto& name = ecs.GetName(entity);

		if (!entitySearchBar.PassFilter(name.c_str())) continue;

		bool isSelected = ImGui::Selectable(name.c_str(), entity == entitySelected, ImGuiSelectableFlags_AllowDoubleClick);

		ImGui::OpenPopupOnItemClick(name.c_str(), ImGuiMouseButton_Right);

		if (ImGui::BeginPopup(name.c_str()))
		{
			ImGui::MenuItem(name.c_str(), nullptr, false, false);
			if (ImGui::MenuItem("Rename"))
			{
				// @todo
			}
			ImGui::EndPopup();
		}

		if (isSelected)
		{
			entitySelected = entity;
		}

		if (prevEntity == entitySelected)
		{
			entityAfterSelected = entity;
		}

		// Focus on double clicked objects.
		if (isSelected && ImGui::IsMouseDoubleClicked(0) && ecs.HasComponent<Bounds>(entity))
		{
			const auto& bounds = ecs.GetComponent<Bounds>(entity);
			const glm::vec3 center = bounds.Box.GetCenter();
			engine.Camera.LookAt(center);
		}

		prevEntity = entity;
	}

	if (engine._Input.GetKeyUp(EKeyCode::Delete))
	{
		ecs.Destroy(entitySelected);
		entitySelected = entityAfterSelected;
	}
	
	ImGui::End();

	if (!ImGui::Begin("Components") || !ecs.IsValid(entitySelected))
	{
		ImGui::End();
		return;
	}

	ImGui::Text(ecs.GetName(entitySelected).c_str());

	if (ecs.HasComponent<Transform>(entitySelected) && ImGui::TreeNode("Transform"))
	{
		auto& transform = ecs.GetComponent<Transform>(entitySelected);
		glm::vec3 position = transform.GetPosition();
		glm::vec3 rotation = transform.GetRotation();
		glm::vec3 scale = transform.GetScale();
		float angle = transform.GetAngle();

		ImGui::DragFloat3("Position", &position[0], 0.05f);
		ImGui::DragFloat3("Rotation", &rotation[0], 0.05f);
		ImGui::DragFloat("Angle", &angle, 1.0f, -180.0, 180.0f);
		ImGui::DragFloat3("Scale", &scale[0], 0.05f);

		if (position != transform.GetPosition() ||
			rotation != transform.GetRotation() ||
			scale != transform.GetScale() ||
			angle != transform.GetAngle())
		{
			transform.Translate(ecs, position);
			transform.Rotate(ecs, rotation, angle);
			transform.Scale(ecs, scale);
		}

		ImGui::TreePop();
	}

	if (ecs.HasComponent<DirectionalLight>(entitySelected) && ImGui::TreeNode("Directional Light"))
	{
		auto& light = ecs.GetComponent<DirectionalLight>(entitySelected);
		glm::vec3 direction = light.Direction;
		glm::vec3 color = light.Color;
		float intensity = light.Intensity;

		ImGui::DragFloat3("Direction", &direction[0]);
		ImGui::ColorEdit3("Color", &color[0]);
		ImGui::DragFloat("Intensity", &intensity, 1.0f, 0.0f);

		if (direction != light.Direction ||
			color != light.Color ||
			intensity != light.Intensity)
		{
			light.Direction = direction;
			light.Color = color;
			light.Intensity = intensity;
		}

		if (ecs.HasComponent<ShadowProxy>(entitySelected))
		{
			ImGui::Text("Shadows");
			auto& shadow = ecs.GetComponent<ShadowProxy>(entitySelected);
			ImGui::DragFloat("Width", &shadow._Width);
			ImGui::DragFloat("ZNear", &shadow._ZNear);
			ImGui::DragFloat("ZFar", &shadow._ZFar);
		}

		ImGui::TreePop();
	}

	if (ecs.HasComponent<SkyboxComponent>(entitySelected) && ImGui::TreeNode("Skybox"))
	{
		auto& device = engine.Device;
		auto& skyboxComp = ecs.GetComponent<SkyboxComponent>(entitySelected);
		Skybox* skybox = skyboxComp.Skybox;
		
		for (uint32 face = CubemapFace_Begin; face != CubemapFace_End; face++)
		{
			gpu::TextureID& textureID = const_cast<gpu::TextureID&>(const_cast<gpu::Image*>(skybox->GetFaces()[face])->GetTextureID());
			ImGui::Text(Skybox::CubemapFaces[face].c_str());
			ImGui::SameLine();
			ImGui::ImageButton(
				&textureID,
				ImVec2(64.0f, 64.0f));
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

ImGuiRenderData::ImGuiRenderData(Engine& engine)
{
	gpu::Device& device = engine.Device;
	ImGuiIO& imgui = ImGui::GetIO();

	unsigned char* pixels;
	int32 width, height;
	imgui.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	fontImage = device.CreateImage(width, height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	samplerID = device.CreateSampler({}).GetSamplerID();

	gpu::UploadImageData(device, pixels, fontImage);

	imgui.Fonts->TexID = &const_cast<gpu::TextureID&>(fontImage.GetTextureID());

	psoDesc.depthStencilState.depthTestEnable = false;
	psoDesc.depthStencilState.depthWriteEnable = false;
	psoDesc.depthStencilState.depthCompareTest = EDepthCompareTest::Always;
	psoDesc.shaderStages.vertex = engine.ShaderLibrary.FindShader<UserInterfaceVS>();
	psoDesc.shaderStages.fragment = engine.ShaderLibrary.FindShader<UserInterfaceFS>();
	psoDesc.colorBlendAttachmentStates.resize(1, {});
	psoDesc.colorBlendAttachmentStates[0].blendEnable = true;
	psoDesc.colorBlendAttachmentStates[0].srcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	psoDesc.colorBlendAttachmentStates[0].dstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	psoDesc.colorBlendAttachmentStates[0].colorBlendOp = EBlendOp::ADD;
	psoDesc.colorBlendAttachmentStates[0].srcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	psoDesc.colorBlendAttachmentStates[0].dstAlphaBlendFactor = EBlendFactor::ZERO;
	psoDesc.colorBlendAttachmentStates[0].alphaBlendOp = EBlendOp::ADD;
	psoDesc.dynamicStates.push_back(EDynamicState::Scissor);
	psoDesc.vertexAttributes = {
		{ 0, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
		{ 1, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
		{ 2, 0, EFormat::R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };
	psoDesc.vertexBindings = { { 0, sizeof(ImDrawVert) } };

	engine._Screen.OnScreenResize([this, &device] (int32 width, int32 height)
	{
		ImGuiIO& imgui = ImGui::GetIO();
		imgui.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

		psoDesc.viewport.width = width;
		psoDesc.viewport.height = height;
	});
}

void ImGuiRenderData::Render(gpu::Device& device, gpu::CommandList& cmdList, const gpu::RenderPass& renderPass)
{
	cmdList.BeginRenderPass(renderPass);

	const ImDrawData* drawData = ImGui::GetDrawData();

	if (drawData->CmdListsCount > 0)
	{
		psoDesc.renderPass = renderPass;

		gpu::Pipeline pipeline = device.CreatePipeline(psoDesc);

		cmdList.BindPipeline(pipeline);

		cmdList.PushConstants(pipeline, psoDesc.shaderStages.vertex, &scaleAndTranslation);

		const std::vector<VkDescriptorSet> descriptorSets = { device.GetTextures().GetSet(), device.GetSamplers().GetSet() };
		cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

		cmdList.BindVertexBuffers(1, &vertexBuffer);

		const ImVec2 clipOff = drawData->DisplayPos;         // (0,0) unless using multi-viewports
		const ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		int32 vertexOffset = 0;
		int32 indexOffset = 0;

		for (int32 cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++)
		{
			const ImDrawList* drawList = drawData->CmdLists[cmdListIndex];

			for (int32 drawCmdIndex = 0; drawCmdIndex < drawList->CmdBuffer.Size; drawCmdIndex++)
			{
				const ImDrawCmd* drawCmd = &drawList->CmdBuffer[drawCmdIndex];
				
				ImVec4 clipRect;
				clipRect.x = std::max((drawCmd->ClipRect.x - clipOff.x) * clipScale.x, 0.0f);
				clipRect.y = std::max((drawCmd->ClipRect.y - clipOff.y) * clipScale.y, 0.0f);
				clipRect.z = (drawCmd->ClipRect.z - clipOff.x) * clipScale.x;
				clipRect.w = (drawCmd->ClipRect.w - clipOff.y) * clipScale.y;

				Scissor scissor;
				scissor.offset.x = static_cast<int32_t>(clipRect.x);
				scissor.offset.y = static_cast<int32_t>(clipRect.y);
				scissor.extent.x = static_cast<uint32_t>(clipRect.z - clipRect.x);
				scissor.extent.y = static_cast<uint32_t>(clipRect.w - clipRect.y);

				cmdList.SetScissor(1, &scissor);

				const glm::uvec2 pushConstants(*static_cast<uint32*>(drawCmd->TextureId), samplerID);
				cmdList.PushConstants(pipeline, psoDesc.shaderStages.fragment, &pushConstants);

				cmdList.DrawIndexed(indexBuffer, drawCmd->ElemCount, 1, drawCmd->IdxOffset + indexOffset, drawCmd->VtxOffset + vertexOffset, 0, EIndexType::UINT16);
			}

			indexOffset += drawList->IdxBuffer.Size;
			vertexOffset += drawList->VtxBuffer.Size;
		}
	}

	cmdList.EndRenderPass();
}

void ImGuiRenderData::Update(gpu::Device& device)
{
	const ImDrawData* drawData = ImGui::GetDrawData();
	const uint32 vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
	const uint32 indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0))
	{
		return;
	}

	vertexBuffer = device.CreateBuffer(EBufferUsage::Vertex | EBufferUsage::HostVisible, vertexBufferSize);
	ImDrawVert* vertexData = static_cast<ImDrawVert*>(vertexBuffer.GetData());
	indexBuffer = device.CreateBuffer(EBufferUsage::Index | EBufferUsage::HostVisible, indexBufferSize);
	ImDrawIdx* indexData = static_cast<ImDrawIdx*>(indexBuffer.GetData());

	for (int32 cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIndex];
		Platform::Memcpy(vertexData, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		Platform::Memcpy(indexData, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertexData += drawList->VtxBuffer.Size;
		indexData += drawList->IdxBuffer.Size;
	}

	ImGuiIO& imgui = ImGui::GetIO();
	
	scaleAndTranslation.x = 2.0f / imgui.DisplaySize.x;
	scaleAndTranslation.y = 2.0f / imgui.DisplaySize.y;
	scaleAndTranslation.z = -1.0f - drawData->DisplayPos.x * scaleAndTranslation.x;
	scaleAndTranslation.w = -1.0f - drawData->DisplayPos.y * scaleAndTranslation.y;
}