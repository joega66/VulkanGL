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
	inline const SpecializationInfo& GetSpecializationInfo() const { return _Material->GetSpecializationInfo(); }
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
	SurfaceGroup(const gpu::DescriptorSet& surfaceSet)
		: _SurfaceSet(&surfaceSet)
	{
	}

	void AddSurface(const Surface& surface)
	{
		_Surfaces.push_back(surface);
	}

	template<bool doFrustumCulling>
	void Draw(
		gpu::Device& device, 
		gpu::CommandList& cmdList,
		std::size_t numDescriptorSets,
		const VkDescriptorSet* descriptorSets,
		std::size_t numDynamicOffsets,
		const uint32* dynamicOffsets,
		std::function<PipelineStateDesc()> getPsoDesc,
		const FrustumPlanes* viewFrustumPlanes = nullptr)
	{
		/** Everything in a SurfaceGroup has the same pipeline layout. Create a dummy pipeline. */
		PipelineStateDesc psoDesc = getPsoDesc();
		gpu::Pipeline pipeline = device.CreatePipeline(psoDesc);

		cmdList.BindDescriptorSets(pipeline, numDescriptorSets, descriptorSets, numDynamicOffsets, dynamicOffsets);

		for (const auto& surface : _Surfaces)
		{
			if constexpr (doFrustumCulling)
			{
				if (Physics::IsBoxInsideFrustum(*viewFrustumPlanes, surface.GetBoundingBox()) == false)
				{
					continue;
				}
			}

			psoDesc = getPsoDesc();
			psoDesc.specInfo = surface.GetSpecializationInfo();

			gpu::Pipeline pipeline = device.CreatePipeline(psoDesc);
			
			cmdList.BindPipeline(pipeline);

			cmdList.PushConstants(pipeline, psoDesc.shaderStages.vertex, &surface.GetSurfaceID());

			cmdList.PushConstants(pipeline, psoDesc.shaderStages.fragment, &surface.GetMaterial()->GetPushConstants());

			for (const auto& submesh : surface.GetSubmeshes())
			{
				cmdList.BindVertexBuffers(static_cast<uint32>(submesh.GetVertexBuffers().size()), submesh.GetVertexBuffers().data());

				cmdList.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
			}
		}
	}

	inline const gpu::DescriptorSet& GetSurfaceSet() const { return *_SurfaceSet; }

private:
	const gpu::DescriptorSet* _SurfaceSet;
	std::vector<Surface> _Surfaces;
};