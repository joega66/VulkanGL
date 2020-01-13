#include "EngineMain.h"
#include <DRMShader.h>
#include <Renderer/SceneRenderer.h>
#include <Renderer/SceneProxy.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/GameSystem.h>
#include <Systems/TransformGizmoSystem.h>

static void CreateDebugMaterials(DRM& Device)
{
	std::vector<uint8> Colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		234, 115, 79, 0 // Dummy
	};

	drm::BufferRef StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Colors.size(), Colors.data());

	Material::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Dummy = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	drm::CommandListRef CmdList = Device.CreateCommandList();

	std::vector<ImageMemoryBarrier> Barriers(5, { nullptr, EAccess::None, EAccess::TransferWrite, EImageLayout::TransferDstOptimal });
	Barriers[0].Image = Material::Red;
	Barriers[1].Image = Material::Green;
	Barriers[2].Image = Material::Blue;
	Barriers[3].Image = Material::White;
	Barriers[4].Image = Material::Dummy;

	CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, Barriers.size(), Barriers.data());

	for (uint32 ColorIndex = 0; ColorIndex < Barriers.size(); ColorIndex++)
	{
		CmdList->CopyBufferToImage(StagingBuffer, ColorIndex * 4 * sizeof(uint8), Barriers[ColorIndex].Image);
	}

	for (auto& Barrier : Barriers)
	{
		Barrier.SrcAccessMask = EAccess::TransferWrite;
		Barrier.DstAccessMask = EAccess::ShaderRead;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, Barriers.size(), Barriers.data());

	Device.SubmitCommands(CmdList);
}

void EngineMain::Main(
	Platform& Platform,
	Cursor& Cursor,
	Input& Input,
	Screen& Screen,
	DRM& Device,
	DRMShaderMap& ShaderMap
)
{
	CreateDebugMaterials(Device);

	Scene Scene(Device, ShaderMap, Cursor, Input, Screen);
	SceneRenderer SceneRenderer(Device, Scene, Screen);
	SystemsManager SystemsManager;

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	GameSystem GameSystem;
	SystemsManager.Register(GameSystem);

	SystemsManager.StartSystems(Scene);

	while (!Platform.WindowShouldClose())
	{
		Platform.PollEvents();

		SystemsManager.UpdateSystems(Scene);
		
		SceneProxy SceneProxy(Device, Scene);

		SceneRenderer.Render(SceneProxy);

		Cursor.Update(Platform);

		Input.Update(Platform);
	}
}