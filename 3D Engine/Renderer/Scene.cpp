#include "Scene.h"
#include <Engine/AssetManager.h>

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

// @todo The Vulkan renderer should hold a copy of in-flight resources
void Scene::RenderLines()
{
	if (Lines.empty())
		return;

	GLShaderRef VertShader = GLCreateShader<LinesVS>();
	GLShaderRef FragShader = GLCreateShader<LinesFS>();

	std::vector<glm::vec3> Positions;

	std::for_each(Lines.begin(), Lines.end(), [&](const auto& Line)
	{
		Positions.push_back(Line.A);
		Positions.push_back(Line.B);
		Positions.push_back(Line.B);
	});

	GLVertexBufferRef VertexBuffer = GLCreateVertexBuffer(IF_R32G32B32_SFLOAT, Positions.size(), EResourceUsage::None, Positions.data());

	GLSetGraphicsPipeline(VertShader, nullptr, nullptr, nullptr, FragShader);

	for (uint32 i = 0; i < Lines.size(); i++)
	{
		GLUniformBufferRef ColorUniform = GLCreateUniformBuffer(Lines[i].Color, EUniformUpdate::SingleFrame);

		GLSetRasterizerState(ECullMode::None, EFrontFace::CCW, EPolygonMode::Line, Lines[i].Width);
		GLSetUniformBuffer(VertShader, "ViewUniform", View.Uniform);
		GLSetUniformBuffer(FragShader, "ColorUniform", ColorUniform);
		GLSetVertexStream(VertShader->GetAttributeLocation("Position"), VertexBuffer);
		GLDraw(3, 1, i * 3, 0);
	}

	GLSetRasterizerState(ECullMode::None);
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