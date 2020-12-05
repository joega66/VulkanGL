#pragma once
#include "GPUDefinitions.h"

#define UNIFORM_STRUCT(StructName, Members)		\
struct StructName								\
{												\
	Members										\
}; static_assert(sizeof(StructName) % 16 == 0);	\

namespace gpu
{
	class Device
	{
	public:
		virtual ~Device() {}

		virtual void EndFrame() = 0;

		virtual void SubmitCommands(gpu::CommandBuffer& cmdBuf) = 0;

		virtual void SubmitCommands(
			gpu::CommandBuffer& cmdBuf,
			const gpu::Semaphore& waitSemaphore,
			const gpu::Semaphore& signalSemaphore
		) = 0;

		virtual gpu::CommandBuffer CreateCommandBuffer(EQueue queue) = 0;

		virtual gpu::Pipeline CreatePipeline(const PipelineStateDesc& psoDesc) = 0;

		virtual gpu::Pipeline CreatePipeline(const ComputePipelineDesc& computePipelineDesc) = 0;

		virtual gpu::Buffer CreateBuffer(
			EBufferUsage bufferUsage, 
			EMemoryUsage memoryUsage,
			uint64 size, 
			const void* data = nullptr
		) = 0;

		virtual gpu::Image CreateImage(
			uint32 width,
			uint32 height,
			uint32 depth,
			EFormat format,
			EImageUsage usageFlags,
			uint32 mipLevels = 1
		) = 0;

		virtual gpu::ImageView CreateImageView(
			const gpu::Image& image,
			uint32 baseMipLevel,
			uint32 levelCount,
			uint32 baseArrayLayer,
			uint32 layerCount
		) = 0;

		virtual gpu::Sampler CreateSampler(const SamplerDesc& samplerDesc) = 0;

		virtual gpu::RenderPass CreateRenderPass(const RenderPassDesc& rpDesc) = 0;

		virtual VkDescriptorSet& GetTextures() = 0;

		virtual VkDescriptorSet& GetImages() = 0;

		virtual gpu::Semaphore CreateSemaphore() = 0;

		template<typename T>
		void UpdateDescriptorSet(const T& descriptors)
		{
			UpdateDescriptorSet(T::_DescriptorSet, T::_DescriptorUpdateTemplate, &descriptors);
		}

		virtual void UpdateDescriptorSet(
			VkDescriptorSet descriptorSet,
			VkDescriptorUpdateTemplate descriptorUpdateTemplate,
			const void* data
		) = 0;

		/** Find shader of ShaderType. */
		template<typename ShaderType>
		const ShaderType* FindShader()
		{
			const std::type_index typeIndex = std::type_index(typeid(ShaderType));
			return static_cast<ShaderType*>(_Shaders[typeIndex]);
		}

		/** Recompile cached shaders. */
		virtual void RecompileShaders() = 0;

	private:
		/** Compile a shader. */
		virtual ShaderCompilationResult CompileShader(
			const ShaderCompilerWorker& worker,
			const std::filesystem::path& path,
			const std::string& entrypoint,
			EShaderStage stage) = 0;

	protected:
		/** Cached shaders. */
		std::unordered_map<std::type_index, gpu::Shader*> _Shaders;
	};

	/** Compositor interface. Useful for creating a swapchain or letting an SDK control the display, e.g. OpenXR. */
	class Compositor
	{
	public:
		virtual uint32 AcquireNextImage(gpu::Device& device, gpu::Semaphore& semaphore) = 0;
		virtual void QueuePresent(gpu::Device& device, uint32 imageIndex, gpu::Semaphore& waitSemaphore) = 0;
		virtual void Resize(gpu::Device& device, uint32 screenWidth, uint32 screenHeight, EImageUsage imageUsage) = 0;
		virtual const std::vector<gpu::Image>& GetImages() = 0;
	};

	void UploadImageData(gpu::Device& device, const void* srcPixels, const gpu::Image& dstImage);
}