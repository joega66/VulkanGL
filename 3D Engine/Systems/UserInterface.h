#pragma once
#include <ECS/System.h>
#include <DRM.h>

class UserInterface : public IRenderSystem
{
public:
	UserInterface(Platform& Platform, DRMDevice& Device, DRMShaderMap& ShaderMap, class Screen& Screen);

	~UserInterface();
	
	virtual void Start(class EntityManager& ECS, class DRMDevice& Device) override;

	virtual void Update(class EntityManager& ECS, class DRMDevice& Device) override;

	void Render(drm::CommandList& CmdList);

private:
	void CreateImGuiRenderResources(DRMDevice& Device);

	void UploadImGuiDrawData(DRMDevice& Device);

	struct ImGuiDescriptors
	{
		drm::ImageRef FontImage;
		SamplerState Sampler;
		drm::BufferRef ImguiUniform;

		static const std::vector<DescriptorTemplateEntry>& GetEntries()
		{
			static const std::vector<DescriptorTemplateEntry> Entries = 
			{ 
				{ 0, 1, EDescriptorType::SampledImage },
				{ 1, 1, EDescriptorType::UniformBuffer }
			};
			return Entries;
		}
	};

	DescriptorSet<ImGuiDescriptors> Descriptors;

	drm::BufferRef VertexBuffer;
	drm::BufferRef IndexBuffer;

	PipelineStateDesc PSODesc;
};