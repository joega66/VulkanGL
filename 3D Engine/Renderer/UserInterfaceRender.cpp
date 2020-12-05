#include "SceneRenderer.h"
#include <Systems/UserInterface.h>
#include <ECS/EntityManager.h>
#include <imgui/imgui.h>

class UserInterfaceVS : public gpu::Shader
{
public:
	UserInterfaceVS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}
};

REGISTER_SHADER(UserInterfaceVS, "../Shaders/UserInterfaceVS.glsl", "main", EShaderStage::Vertex);

class UserInterfaceFS : public gpu::Shader
{
public:
	UserInterfaceFS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}
};

REGISTER_SHADER(UserInterfaceFS, "../Shaders/UserInterfaceFS.glsl", "main", EShaderStage::Fragment);

void SceneRenderer::CreateUserInterfacePipeline()
{
	PipelineStateDesc psoDesc = {};
	psoDesc.renderPass = _UserInterfaceRP.front();
	psoDesc.depthStencilState.depthTestEnable = false;
	psoDesc.depthStencilState.depthWriteEnable = false;
	psoDesc.depthStencilState.depthCompareTest = ECompareOp::Always;
	psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<UserInterfaceVS>();
	psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<UserInterfaceFS>();
	psoDesc.colorBlendAttachmentStates.push_back({
		.blendEnable = true,
		.srcColorBlendFactor = EBlendFactor::SRC_ALPHA,
		.dstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = EBlendOp::ADD,
		.srcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA,
		.dstAlphaBlendFactor = EBlendFactor::ZERO,
		.alphaBlendOp = EBlendOp::ADD,
	});
	psoDesc.vertexAttributes = {
		{ 0, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
		{ 1, 0, EFormat::R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
		{ 2, 0, EFormat::R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };
	psoDesc.vertexBindings = { { 0, sizeof(ImDrawVert) } };

	auto& userInterfaceRenderData = _ECS.GetSingletonComponent<UserInterfaceRenderData>();
	userInterfaceRenderData.pipeline = _Device.CreatePipeline(psoDesc);
}

void SceneRenderer::RenderUserInterface(gpu::CommandList& cmdList, const gpu::RenderPass& renderPass)
{
	auto& userInterfaceRenderData = _ECS.GetSingletonComponent<UserInterfaceRenderData>();

	cmdList.BeginRenderPass(renderPass);

	cmdList.SetViewport({ .width = renderPass.GetRenderArea().extent.width, .height = renderPass.GetRenderArea().extent.height });

	const ImDrawData* drawData = ImGui::GetDrawData();
	const auto* vertex = _ShaderLibrary.FindShader<UserInterfaceVS>();
	const auto* fragment = _ShaderLibrary.FindShader<UserInterfaceFS>();

	if (drawData->CmdListsCount > 0)
	{
		cmdList.BindPipeline(userInterfaceRenderData.pipeline);

		cmdList.PushConstants(userInterfaceRenderData.pipeline, vertex, &userInterfaceRenderData.scaleAndTranslation);

		const VkDescriptorSet descriptorSets[] = { _Device.GetTextures() };

		cmdList.BindDescriptorSets(userInterfaceRenderData.pipeline, std::size(descriptorSets), descriptorSets, 0, nullptr);

		cmdList.BindVertexBuffers(1, &userInterfaceRenderData.vertexBuffer);

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

				cmdList.SetScissor({
					.offset = { static_cast<int32_t>(clipRect.x), static_cast<int32_t>(clipRect.y)},
					.extent = { static_cast<uint32_t>(clipRect.z - clipRect.x), static_cast<uint32_t>(clipRect.w - clipRect.y) }
					});

				const uint32 pushConstants(*static_cast<uint32*>(drawCmd->TextureId));
				cmdList.PushConstants(userInterfaceRenderData.pipeline, fragment, &pushConstants);

				cmdList.DrawIndexed(userInterfaceRenderData.indexBuffer, drawCmd->ElemCount, 1, drawCmd->IdxOffset + indexOffset, drawCmd->VtxOffset + vertexOffset, 0, EIndexType::UINT16);
			}

			indexOffset += drawList->IdxBuffer.Size;
			vertexOffset += drawList->VtxBuffer.Size;
		}
	}

	cmdList.EndRenderPass();
}