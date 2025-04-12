#pragma once

#include "RenderDeviceChild.h"
#include "RenderResourceState.h"
#include "PipelineState.h"
#include "RenderPass.h"

#include <vulkan/vulkan_core.h>
#include <glm/vec2.hpp>

#include <limits>
#include <vector>
#include <span>

namespace ZE::RenderBackend
{
	class RenderDevice;
	class PipelineState;

	class Buffer;

	struct PipelineResourceAccessInfo
	{
		// Describes which stage in the pipeline this resource is used.
		VkPipelineStageFlags			m_StageMask;
		// Describes which access mode in the pipeline this resource is used.
		VkAccessFlags					m_AccessMask;
		// Describes the image memory layout which image will be used if this resource is an image resource.
		VkImageLayout					m_ImageLayout;
	};

	struct GlobalMemoryBarrierTransition
	{
		VkPipelineStageFlags				m_SrcStage;
		VkPipelineStageFlags				m_DstStage;
		VkMemoryBarrier						m_Barrier;
	};

	struct BufferBarrierTransition
	{
		VkPipelineStageFlags				m_SrcStage;
		VkPipelineStageFlags				m_DstStage;
		VkBufferMemoryBarrier				m_Barrier;
	};

	struct TextureBarrierTransition
	{
		VkPipelineStageFlags				m_SrcStage;
		VkPipelineStageFlags				m_DstStage;
		VkImageMemoryBarrier				m_Barrier;
	};

	GlobalMemoryBarrierTransition GetMemoryBarrierTransition(const GlobalMemoryBarrier& globalBarrier);
	BufferBarrierTransition GetBufferBarrierTransition(const BufferBarrier& bufferBarrier);
	TextureBarrierTransition GetTextureBarrierTransition(const TextureBarrier& textureBarrier);

	VkImageLayout GetTextureLayout(ERenderResourceState state);
	
	struct DynamicRenderingInfo
	{
		glm::uvec2											m_ViewportSize{0u, 0u};
		std::vector<RenderPassRenderTargetBinding>			m_Attachments;
	};
	
	class RenderCommandList : public RenderDeviceChild
	{
		friend class RenderDevice;

	public:

		RenderCommandList(RenderDevice& renderDevice);
		virtual ~RenderCommandList();

		uint32_t GetQueueIndex() const { return m_QueueIndex; }
		
		bool BeginRecord();
		void EndRecord();
		void Reset() const;

		// state commands
		void CmdBeginDynamicRendering(const glm::uvec2& viewportSize, std::span<Texture*> renderTargets, std::span<RenderPassRenderTargetBinding> colorBindings) const;
		void CmdEndDynamicRendering() const;
		void CmdSetViewport(const glm::uvec2& viewportSize) const;
		// TODO: multi-scissors and offsets
		void CmdSetScissor(const glm::uvec2& viewportSize) const;
		
		// draw commands
		void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;
		void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const;
		
		// pipeline resource commands
		void CmdBindPipeline(VkPipelineBindPoint bindPoint, PipelineState* pPipelineState) const;
		void CmdBindShaderResource(VkPipelineBindPoint bindPoint, PipelineState* pPipelineState, const std::vector<VkDescriptorSet>& sets) const;
		void CmdBindVertexInput(const Buffer* pVertexBuffer, const Buffer* pIndexBuffer = nullptr) const;

		// barrier commands
		void CmdResourceBarrier(const GlobalMemoryBarrier* pMemoryBarrier, std::span<const BufferBarrier> pBufferBarriers, std::span<const TextureBarrier> pTextureBarriers) const;
		
		// transfer commands
		void CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t srcOffset = 0) const;

	private:
		
		VkCommandPool					m_CommandPool = nullptr;
		VkCommandBuffer					m_CommandBuffer = nullptr;
		bool							m_IsCommandRecording = false;

		uint32_t						m_QueueIndex = std::numeric_limits<uint32_t>::max();
	};
}
