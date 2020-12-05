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
#include <Components/SkyboxComponent.h>
#include <Systems/SceneSystem.h>
#include <Renderer/ShadowProxy.h>

#define SHOW_COMPONENT(type, ecs, entity, callback)					\
	if (ecs.HasComponent<type>(entity) && ImGui::TreeNode(#type))	\
	{																\
		callback(ecs.GetComponent<type>(entity));					\
		ImGui::TreePop();											\
	}																\

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

	engine._ECS.AddSingletonComponent<UserInterfaceRenderData>(engine._Device, engine.ShaderLibrary);

	_ScreenResizeEvent = engine._Screen.OnScreenResize([&] (int32 width, int32 height)
	{
		ImGuiIO& imgui = ImGui::GetIO();
		imgui.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	});
}

void UserInterface::Update(Engine& engine)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ShowUI(engine);

	ImGui::ShowDemoWindow();
	ImGui::Render();

	engine._ECS.GetSingletonComponent<UserInterfaceRenderData>().Update(engine._Device);
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
		if (isSelected && ImGui::IsMouseDoubleClicked(0) && ecs.HasComponent<StaticMeshComponent>(entity))
		{
			const auto& transform = ecs.GetComponent<Transform>(entity);
			const auto& staticMesh = ecs.GetComponent<StaticMeshComponent>(entity);
			const BoundingBox boundingBox = staticMesh.StaticMesh->GetBounds().Transform(transform.GetLocalToWorld());

			for (auto entity : ecs.GetEntities<Camera>())
			{
				auto& camera = ecs.GetComponent<Camera>(entity);
				camera.LookAt(boundingBox.GetCenter());
			}
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

	ImGui::SameLine();

	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("AddComponent");
	}

	if (ImGui::BeginPopup("AddComponent"))
	{
		if (ImGui::MenuItem("Directional Light"))
		{
			ecs.AddComponent<DirectionalLight>(entitySelected, DirectionalLight{});
		}

		ImGui::EndPopup();
	}

	SHOW_COMPONENT(Transform, ecs, entitySelected, [&](auto& transform)
	{
		glm::vec3 position = transform.GetPosition();
		glm::vec3 eulerAngles = transform.GetEulerAngles();
		glm::vec3 scale = transform.GetScale();

		eulerAngles = glm::degrees(eulerAngles);

		static constexpr float translationSpeed = 0.05f;
		static constexpr float rotationSpeed = 0.05f;
		static constexpr float scaleSpeed = 0.05f;

		ImGui::DragFloat3("Position", &position[0], translationSpeed);
		ImGui::DragFloat3("Rotation", &eulerAngles[0], rotationSpeed);
		ImGui::DragFloat3("Scale", &scale[0], scaleSpeed);

		eulerAngles = glm::radians(eulerAngles);

		if (position	!= transform.GetPosition() ||
			eulerAngles != transform.GetEulerAngles() ||
			scale		!= transform.GetScale())
		{			
			transform.Translate(ecs, position);
			transform.Rotate(ecs, eulerAngles);
			transform.Scale(ecs, scale);
		}
	});

	SHOW_COMPONENT(DirectionalLight, ecs, entitySelected, [&] (auto& light)
	{
		glm::vec3 color = light.Color;
		float intensity = light.Intensity;

		ImGui::ColorEdit3("Color", &color[0]);
		ImGui::DragFloat("Intensity", &intensity, 1.0f, 0.0f);

		if (color != light.Color ||
			intensity != light.Intensity)
		{
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
	});

	_UserTextures.clear();

	SHOW_COMPONENT(SkyboxComponent, ecs, entitySelected, [&] (auto& skyboxComp)
	{
		Skybox* skybox = skyboxComp.Skybox;

		for (uint32 face = CubemapFace_Begin; face != CubemapFace_End; face++)
		{
			_UserTextures.push_back( skybox->GetFaces()[face]->GetTextureID(engine._Device.CreateSampler({})) );
			ImGui::Text(Skybox::CubemapFaces[face].c_str());
			ImGui::SameLine();
			ImGui::ImageButton(
				&_UserTextures.back(),
				ImVec2(64.0f, 64.0f)
			);
		}
	});

	SHOW_COMPONENT(Camera, ecs, entitySelected, [&] (auto& camera)
	{
		float fieldOfView = camera.GetFieldOfView();
		ImGui::InputFloat("Field of view", &fieldOfView, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
		
		if (fieldOfView != camera.GetFieldOfView())
		{
			camera.SetFieldOfView(fieldOfView);
		}
	});

	ImGui::End();
}

UserInterfaceRenderData::UserInterfaceRenderData(gpu::Device& device, gpu::ShaderLibrary& shaderLibrary)
{
	ImGuiIO& imgui = ImGui::GetIO();

	unsigned char* pixels;
	int32 width, height;
	imgui.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	fontImage = device.CreateImage(width, height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	
	gpu::UploadImageData(device, pixels, fontImage);

	fontTexture = fontImage.GetTextureID(device.CreateSampler({}));

	imgui.Fonts->TexID = &fontTexture;
}

void UserInterfaceRenderData::Update(gpu::Device& device)
{
	const ImDrawData* drawData = ImGui::GetDrawData();
	const uint32 vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
	const uint32 indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0))
	{
		return;
	}

	vertexBuffer = device.CreateBuffer(EBufferUsage::Vertex, EMemoryUsage::CPU_TO_GPU, vertexBufferSize);
	ImDrawVert* vertexData = static_cast<ImDrawVert*>(vertexBuffer.GetData());
	indexBuffer = device.CreateBuffer(EBufferUsage::Index, EMemoryUsage::CPU_TO_GPU, indexBufferSize);
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