#include <Renderer/SceneRenderer.h>
#include <Renderer/SceneProxy.h>
#include <Systems/EditorControllerSystem.h>
#include <Systems/GameSystem.h>
#include <Systems/TransformGizmoSystem.h>
#include <Systems/RenderSystem.h>
#include <Systems/UserInterface.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include "EngineMain.h"

static void CreateDebugMaterials(DRMDevice& Device)
{
	std::vector<uint8> Colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		0, 0, 0, 0, // Black
		234, 115, 79, 0 // Dummy
	};

	drm::BufferRef StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Colors.size(), Colors.data());

	Material::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Black = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Dummy = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	std::vector<ImageMemoryBarrier> Barriers
	(
		Colors.size() / 4,
		{ nullptr, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal }
	);

	Barriers[0].Image = Material::Red;
	Barriers[1].Image = Material::Green;
	Barriers[2].Image = Material::Blue;
	Barriers[3].Image = Material::White;
	Barriers[4].Image = Material::Black;
	Barriers[5].Image = Material::Dummy;

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, Barriers.size(), Barriers.data());

	for (uint32 ColorIndex = 0; ColorIndex < Barriers.size(); ColorIndex++)
	{
		CmdList.CopyBufferToImage(StagingBuffer, ColorIndex * 4 * sizeof(uint8), Barriers[ColorIndex].Image, EImageLayout::TransferDstOptimal);
	}

	for (auto& Barrier : Barriers)
	{
		Barrier.SrcAccessMask = EAccess::TransferWrite;
		Barrier.DstAccessMask = EAccess::ShaderRead;
		Barrier.OldLayout = EImageLayout::TransferDstOptimal;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, Barriers.size(), Barriers.data());

	Device.SubmitCommands(CmdList);
}

Engine::Engine(
	Platform& InPlatform, 
	Cursor& InCursor,
	Input& InInput,
	Screen& InScreen,
	DRMDevice& InDevice, 
	DRMShaderMap& InShaderMap,
	drm::Surface& InSurface
) : _Platform(InPlatform)
	, _Cursor(InCursor)
	, _Input(InInput)
	, _Screen(InScreen)
	, _Device(InDevice)
	, _ShaderMap(InShaderMap)
	, _Surface(InSurface)
{
}

void Engine::Main()
{
	CreateDebugMaterials(_Device);

	Scene Scene(_Device, _ShaderMap, _Cursor, _Input, _Screen);
	SceneRenderer SceneRenderer(_Device, _Surface, Scene, _Screen);

	SystemsManager SystemsManager;

	RenderSystem RenderSystem(_Device);
	SystemsManager.Register(RenderSystem);

	EditorControllerSystem EditorControllerSystem;
	SystemsManager.Register(EditorControllerSystem);

	GameSystem GameSystem;
	SystemsManager.Register(GameSystem);

	UserInterface UserInterface(_Platform, _Device, _ShaderMap, _Screen);
	SystemsManager.Register(UserInterface);

	SystemsManager.StartRenderSystems(Scene.ECS, _Device);
	SystemsManager.StartSystems(Scene);

	while (!_Platform.WindowShouldClose())
	{
		_Platform.PollEvents();

		SystemsManager.UpdateSystems(Scene);

		Scene.ECS.NotifyComponentEvents();

		SystemsManager.UpdateRenderSystems(Scene.ECS, _Device);

		SceneProxy SceneProxy(_Device, Scene);

		SceneRenderer.Render(UserInterface, SceneProxy);

		_Cursor.Update(_Platform);

		_Input.Update(_Platform);
	}
}