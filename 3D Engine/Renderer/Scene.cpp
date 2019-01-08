#include "Scene.h"
#include <Engine/ResourceManager.h>

Scene::Scene()
{
	SceneDepth = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT, EResourceUsage::RenderTargetable);
	OutlineDepthStencil = GLCreateImage(GPlatform->GetWindowSize().x, GPlatform->GetWindowSize().y, IF_D32_SFLOAT_S8_UINT, EResourceUsage::RenderTargetable);

	Skybox = GAssetManager.GetCubemap("Engine-Cubemap-Default");
}

void Scene::Render()
{
	RenderLightingPass();
	RenderEditorPrimitives();
}

void Scene::DrawLine(const DrawLineInfo& DrawLineInfo)
{
	Lines.push_back(DrawLineInfo);
}

void Scene::RenderLightingPass()
{
	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Clear, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef DepthView = GLCreateRenderTargetView(SceneDepth, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);

	GLSetRenderTargets(1, &SurfaceView, DepthView, EDepthStencilAccess::DepthWrite);
	GLSetViewport(0.0f, 0.0f, (float)GPlatform->GetWindowSize().x, (float)GPlatform->GetWindowSize().y);
	GLSetDepthTest(true);
	GLSetColorMask(0, EColorChannel::RGBA);
	GLSetRasterizerState(ECullMode::None);

	LightingPassDrawingPlans.Execute(View);
}

void Scene::RenderEditorPrimitives()
{
	RenderLines();
	RenderSkybox();
	RenderOutlines();
}

void Scene::RenderLines()
{
	// @todo Create one big vertex buffer and use FirstVertex param.

	//GLSetGraphicsPipeline()
	std::for_each(Lines.begin(), Lines.end(), [&](const auto& Line)
	{
		GLSetRasterizerState(ECullMode::None, EFrontFace::CCW, EPolygonMode::Line, Line.Width);
	});
}

void Scene::RenderSkybox()
{
	GLShaderRef VertShader = GLCreateShader<SkyboxVS>();
	GLShaderRef FragShader = GLCreateShader<SkyboxFS>();
	StaticMeshRef Cube = GAssetManager.GetStaticMesh("Cube");

	GLSetDepthTest(true, EDepthCompareTest::LEqual);
	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);
	GLSetUniformBuffer(VertShader, VertShader->GetUniformLocation("ViewUniform"), View.Uniform);
	GLSetShaderImage(FragShader, FragShader->GetUniformLocation("Skybox"), Skybox, SamplerState{EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear});

	for (const auto& Resource : Cube->Resources)
	{
		GLSetVertexStream(VertShader->GetAttributeLocation("Position"), Resource.PositionBuffer);
		GLDrawIndexed(Resource.IndexBuffer, Resource.IndexCount, 1, 0, 0, 0);
	}

	GLSetDepthTest(true);
}

void Scene::RenderOutlines()
{
	GLRenderTargetViewRef StencilView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Clear, EStoreAction::Store, 1.0f, 0);

	GLSetRenderTargets(0, nullptr, StencilView, EDepthStencilAccess::DepthWriteStencilWrite);
	GLSetStencilTest(true);
	GLSetStencilState(ECompareOp::Always, EStencilOp::Replace, EStencilOp::Replace, EStencilOp::Replace, 0xff, 0xff, 1);

	Stencil.Execute(View);

	GLRenderTargetViewRef SurfaceView = GLGetSurfaceView(ELoadAction::Load, EStoreAction::Store, { 0, 0, 0, 0 });
	GLRenderTargetViewRef OutlineView = GLCreateRenderTargetView(OutlineDepthStencil, ELoadAction::Load, EStoreAction::Store, 1.0f, 0);

	GLSetRenderTargets(1, &SurfaceView, OutlineView, EDepthStencilAccess::StencilWrite);
	GLSetDepthTest(false);
	GLSetStencilState(ECompareOp::NotEqual, EStencilOp::Keep, EStencilOp::Keep, EStencilOp::Replace, 0xff, 0, 1);

	Outline.Execute(View);

	GLSetStencilTest(false);
	GLSetDepthTest(true);
}

Scene& Scene::Get()
{
	static Scene Scene;
	return Scene;
}