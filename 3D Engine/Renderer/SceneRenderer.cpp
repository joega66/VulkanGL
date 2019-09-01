#include "SceneRenderer.h"
#include "Scene.h"
#include "FullscreenQuad.h"
#include "RayMarching.h"
#include "Light.h"
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

SceneRenderer::SceneRenderer()
{
	Screen.RegisterScreenResChangedCallback([&] (int32 Width, int32 Height)
	{
		SceneDepth = drm::CreateImage(Width, Height, EImageFormat::D16_UNORM, EResourceUsage::RenderTargetable);
		OutlineDepthStencil = drm::CreateImage(Width, Height, EImageFormat::D24_UNORM_S8_UINT, EResourceUsage::RenderTargetable);
	});
}

void SceneRenderer::Render(Scene& Scene)
{
	InitView(Scene);

	drm::BeginFrame();

	RenderCommandListRef CmdList = drm::CreateCommandList();

	{
		drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColorValue{});
		drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{}, EImageLayout::DepthWriteStencilWrite);

		RenderPassInitializer RenderPassInit = { 1 };
		RenderPassInit.ColorTargets[0] = SurfaceView;
		RenderPassInit.DepthTarget = DepthView;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

		CmdList->BeginRenderPass(RenderPassInit);

		RenderLightingPass(Scene, *CmdList);
		//RenderRayMarching(*CmdList);
		RenderLines(Scene, *CmdList);
		RenderSkybox(Scene, *CmdList);

		CmdList->EndRenderPass();
	}

	RenderOutlines(Scene, *CmdList);

	CmdList->Finish();

	drm::SubmitCommands(CmdList);

	drm::EndFrame();
}

void SceneRenderer::InitView(Scene& Scene)
{
	// Initialize view uniform.
	View& View = Scene.View;

	const glm::mat4 WorldToView = View.GetWorldToView();
	const glm::mat4 ViewToClip = View.GetViewToClip();
	const glm::mat4 WorldToClip = ViewToClip * WorldToView;

	struct ViewUniformData
	{
		glm::mat4 WorldToView;
		glm::mat4 ViewToClip;
		glm::mat4 WorldToClip;
		glm::vec3 Position;
		float _Pad0;
		float AspectRatio;
		float FOV;
		glm::vec2 _Pad1;
	};

	const ViewUniformData ViewUniformData =
	{
		WorldToView,
		ViewToClip,
		WorldToClip,
		View.GetPosition(),
		0.0f,
		(float)Screen.GetWidth() / Screen.GetHeight(),
		View.GetFOV()
	};

	ViewUniform = drm::CreateUniformBuffer(sizeof(ViewUniformData), &ViewUniformData, EUniformUpdate::SingleFrame);

	// Initialize light buffer.
	glm::uvec4 NumPointLights;
	NumPointLights.x = Scene.PointLightProxies.size();

	PointLightBuffer = drm::CreateStorageBuffer(sizeof(NumPointLights) + sizeof(PointLightProxy) * Scene.PointLightProxies.size(), nullptr);

	void* Data = drm::LockBuffer(PointLightBuffer);
	Platform.Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform.Memcpy((uint8*)Data + sizeof(NumPointLights), Scene.PointLightProxies.data(), sizeof(PointLightProxy) * Scene.PointLightProxies.size());
	drm::UnlockBuffer(PointLightBuffer);
}

void SceneRenderer::RenderRayMarching(Scene& Scene, RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	Ref<FullscreenVS> VertShader = *ShaderMapRef<FullscreenVS>();
	Ref<RayMarchingFS> FragShader = *ShaderMapRef<RayMarchingFS>();

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	SetResources(CmdList, FragShader, FragShader->SceneBindings);

	CmdList.Draw(3, 1, 0, 0);
}

void SceneRenderer::RenderLightingPass(Scene& Scene, RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	Scene.LightingPass.Draw(CmdList, PSOInit, *this);
}

void SceneRenderer::RenderLines(Scene& Scene, RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.RasterizationState.PolygonMode = EPolygonMode::Line;

	Scene.Lines.Draw(CmdList, PSOInit, *this);
}

void SceneRenderer::RenderSkybox(Scene& Scene, RenderCommandList& CmdList)
{
	StaticMeshRef Cube = GAssetManager.GetStaticMesh("Cube");

	Ref<SkyboxVS> VertShader = *ShaderMapRef<SkyboxVS>();
	Ref<SkyboxFS> FragShader = *ShaderMapRef<SkyboxFS>();

	PipelineStateInitializer PSOInit = {};

	PSOInit.Viewport.Width = Screen.GetWidth();
	PSOInit.Viewport.Height = Screen.GetHeight();

	PSOInit.GraphicsPipelineState = { VertShader, nullptr, nullptr, nullptr, FragShader };

	CmdList.BindPipeline(PSOInit);

	CmdList.SetUniformBuffer(VertShader, VertShader->View, ViewUniform);
	CmdList.SetShaderImage(FragShader, FragShader->Skybox, Scene.Skybox, SamplerState{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	for (const auto& Element : Cube->Batch.Elements)
	{
		CmdList.BindVertexBuffers(1, &Element.GetPositionBuffer());
		CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
	}
}

void SceneRenderer::RenderOutlines(Scene& Scene, RenderCommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};

	{
		drm::RenderTargetViewRef StencilView = drm::CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, ClearDepthStencilValue{}, EImageLayout::DepthWriteStencilWrite);

		RenderPassInitializer RenderPassInit = {};
		RenderPassInit.DepthTarget = StencilView;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(OutlineDepthStencil->Width, OutlineDepthStencil->Height) };

		CmdList.BeginRenderPass(RenderPassInit);

		PSOInit.Viewport.Width = Screen.GetWidth();
		PSOInit.Viewport.Height = Screen.GetHeight();

		PSOInit.DepthStencilState.StencilTestEnable = true;

		PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::Always;
		PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.CompareMask = 0xff;
		PSOInit.DepthStencilState.Back.WriteMask = 0xff;
		PSOInit.DepthStencilState.Back.Reference = 1;

		Scene.Stencil.Draw(CmdList, PSOInit, *this);

		CmdList.EndRenderPass();
	}

	{
		drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Load, EStoreAction::Store, ClearColorValue{});
		drm::RenderTargetViewRef OutlineView = drm::CreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthWriteStencilWrite);

		RenderPassInitializer RenderPassInit = { 1 };
		RenderPassInit.ColorTargets[0] = SurfaceView;
		RenderPassInit.DepthTarget = OutlineView;
		RenderPassInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(OutlineDepthStencil->Width, OutlineDepthStencil->Height) };

		CmdList.BeginRenderPass(RenderPassInit);

		PSOInit.DepthStencilState.DepthTestEnable = false;
		PSOInit.DepthStencilState.DepthWriteEnable = false;

		PSOInit.DepthStencilState.Back.CompareOp = ECompareOp::NotEqual;
		PSOInit.DepthStencilState.Back.FailOp = EStencilOp::Keep;
		PSOInit.DepthStencilState.Back.DepthFailOp = EStencilOp::Keep;
		PSOInit.DepthStencilState.Back.PassOp = EStencilOp::Replace;
		PSOInit.DepthStencilState.Back.CompareMask = 0xff;
		PSOInit.DepthStencilState.Back.WriteMask = 0;
		PSOInit.DepthStencilState.Back.Reference = 1;

		Scene.Outline.Draw(CmdList, PSOInit, *this);

		CmdList.EndRenderPass();
	}
}