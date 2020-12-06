#pragma once
#include <ECS/Component.h>
#include <GPU/GPU.h>
#include <Engine/StaticMesh.h>
#include <Physics/Physics.h>

class Surface
{
public:
	Surface(uint32 surfaceID, const Material* material, const std::vector<Submesh>& submeshes, const BoundingBox& boundingBox)
		: _SurfaceID(surfaceID)
		, _Material(material)
		, _Submeshes(&submeshes)
		, _BoundingBox(boundingBox)
	{
	}

	inline const uint32& GetSurfaceID() const { return _SurfaceID; }
	inline const std::vector<Submesh>& GetSubmeshes() const { return *_Submeshes; }
	inline const Material* GetMaterial() const { return _Material; }
	inline const SpecializationInfo& GetMaterialInfo() const { return _Material->GetSpecializationInfo(); }
	inline const BoundingBox& GetBoundingBox() const { return _BoundingBox; }
	
private:
	uint32 _SurfaceID;
	const std::vector<Submesh>* _Submeshes;
	const Material* _Material;
	BoundingBox _BoundingBox;
};

class SurfaceGroup : public Component
{
public:
	SurfaceGroup(const VkDescriptorSet& surfaceSet)
		: _SurfaceSet(surfaceSet)
	{
	}

	void AddSurface(const Surface& surface)
	{
		_Surfaces.push_back(surface);
	}

	template<bool doFrustumCulling>
	void Draw(
		gpu::Device& device, 
		gpu::CommandBuffer& cmdBuf,
		std::size_t numDescriptorSets,
		const VkDescriptorSet* descriptorSets,
		std::size_t numDynamicOffsets,
		const uint32* dynamicOffsets,
		std::function<GraphicsPipelineDesc()> getPsoDesc,
		const FrustumPlanes* viewFrustumPlanes = nullptr)
	{
		// @todo Everything in a SurfaceGroup has the same pipeline layout. */
		//cmdBuf.BindDescriptorSets(pipeline, numDescriptorSets, descriptorSets, numDynamicOffsets, dynamicOffsets);

		for (const auto& surface : _Surfaces)
		{
			if constexpr (doFrustumCulling)
			{
				if (Physics::IsBoxInsideFrustum(*viewFrustumPlanes, surface.GetBoundingBox()) == false)
				{
					continue;
				}
			}

			GraphicsPipelineDesc graphicsDesc = getPsoDesc();
			graphicsDesc.specInfo = surface.GetMaterialInfo();

			gpu::Pipeline pipeline = device.CreatePipeline(graphicsDesc);
			
			cmdBuf.BindPipeline(pipeline);

			cmdBuf.BindDescriptorSets(pipeline, numDescriptorSets, descriptorSets, numDynamicOffsets, dynamicOffsets);

			cmdBuf.PushConstants(pipeline, graphicsDesc.shaderStages.vertex, &surface.GetSurfaceID());

			cmdBuf.PushConstants(pipeline, graphicsDesc.shaderStages.fragment, &surface.GetMaterial()->GetPushConstants());

			for (const auto& submesh : surface.GetSubmeshes())
			{
				cmdBuf.BindVertexBuffers(static_cast<uint32>(submesh.GetVertexBuffers().size()), submesh.GetVertexBuffers().data());

				cmdBuf.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
			}
		}
	}

	inline const VkDescriptorSet& GetSurfaceSet() const { return _SurfaceSet; }

private:
	VkDescriptorSet _SurfaceSet;
	std::vector<Surface> _Surfaces;
};