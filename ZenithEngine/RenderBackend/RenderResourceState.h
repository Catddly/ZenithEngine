#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <limits>
#include <span>

namespace ZE::RenderBackend
{
	//-------------------------------------------------------------------------
	//	Rather than use sophisticated enum and bit flags inside vulkan or DX12
	//	to perform resource barrier transform in order to synchronize. Render
	//	graph resource barrier simplify this process and erase some of the invalid 
	//	and nonsensical combinations of resource barrier.
	// 
	//	This idea comes from vk-sync-rs (by Graham Wihlidal) and be slightly modified.
	//-------------------------------------------------------------------------

	enum class ERenderResourceState : uint8_t
	{
		/// Undefined resource state, primarily use for initialization.
		Undefined = 0,
		/// Read as an indirect buffer for drawing or dispatch
		IndirectBuffer,
		/// Read as a vertex buffer for drawing
		VertexBuffer,
		/// Read as an index buffer for drawing
		IndexBuffer,

		/// Read as a uniform buffer in a vertex shader
		VertexShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a vertex shader
		VertexShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource in a vertex shader
		VertexShaderReadOther,

		/// Read as a uniform buffer in a tessellation control shader
		TessellationControlShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a tessellation control shader
		TessellationControlShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource in a tessellation control shader
		TessellationControlShaderReadOther,

		/// Read as a uniform buffer in a tessellation evaluation shader
		TessellationEvaluationShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a tessellation evaluation shader
		TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource in a tessellation evaluation shader
		TessellationEvaluationShaderReadOther,

		/// Read as a uniform buffer in a geometry shader
		GeometryShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a geometry shader
		GeometryShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource in a geometry shader
		GeometryShaderReadOther,

		/// Read as a uniform buffer in a fragment shader
		FragmentShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a fragment shader
		FragmentShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as an input attachment with a color format in a fragment shader
		FragmentShaderReadColorInputAttachment,
		/// Read as an input attachment with a depth/stencil format in a fragment shader
		FragmentShaderReadDepthStencilInputAttachment,
		/// Read as any other resource in a fragment shader
		FragmentShaderReadOther,

		/// Read by blending/logic operations or subpass load operations
		ColorAttachmentRead,
		/// Read by depth/stencil tests or subpass load operations
		DepthStencilAttachmentRead,

		/// Read as a uniform buffer in a compute shader
		ComputeShaderReadUniformBuffer,
		/// Read as a sampled image/uniform texel buffer in a compute shader
		ComputeShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource in a compute shader
		ComputeShaderReadOther,

		/// Read as a uniform buffer in any shader
		AnyShaderReadUniformBuffer,
		/// Read as a uniform buffer in any shader, or a vertex buffer
		AnyShaderReadUniformBufferOrVertexBuffer,
		/// Read as a sampled image in any shader
		AnyShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as any other resource (excluding attachments) in any shader
		AnyShaderReadOther,

		/// Read as the source of a transfer operation
		TransferRead,
		/// Read on the host
		HostRead,
		/// Read by the presentation engine (i.e. `vkQueuePresentKHR`)
		Present,

		/// Written as any resource in a vertex shader
		VertexShaderWrite,
		/// Written as any resource in a tessellation control shader
		TessellationControlShaderWrite,
		/// Written as any resource in a tessellation evaluation shader
		TessellationEvaluationShaderWrite,
		/// Written as any resource in a geometry shader
		GeometryShaderWrite,
		/// Written as any resource in a fragment shader
		FragmentShaderWrite,

		/// Written as a color attachment during rendering, or via a subpass store op
		ColorAttachmentWrite,
		/// Written as a depth/stencil attachment during rendering, or via a subpass store op
		DepthStencilAttachmentWrite,
		/// Written as a depth aspect of a depth/stencil attachment during rendering, whilst the
		/// stencil aspect is read-only. Requires `VK_KHR_maintenance2` to be enabled.
		DepthAttachmentWriteStencilReadOnly,
		/// Written as a stencil aspect of a depth/stencil attachment during rendering, whilst the
		/// depth aspect is read-only. Requires `VK_KHR_maintenance2` to be enabled.
		StencilAttachmentWriteDepthReadOnly,

		/// Written as any resource in a compute shader
		ComputeShaderWrite,

		/// Written as any resource in any shader
		AnyShaderWrite,
		/// Written as the destination of a transfer operation
		TransferWrite,
		/// Written on the host
		HostWrite,

		/// Read or written as a color attachment during rendering
		ColorAttachmentReadWrite,
		/// Covers any access - useful for debug, generally avoid for performance reasons
		General,

		/// Read as a sampled image/uniform texel buffer in a ray tracing shader
		RayTracingShaderReadSampledImageOrUniformTexelBuffer,
		/// Read as an input attachment with a color format in a ray tracing shader
		RayTracingShaderReadColorInputAttachment,
		/// Read as an input attachment with a depth/stencil format in a ray tracing shader
		RayTracingShaderReadDepthStencilInputAttachment,
		/// Read as an acceleration structure in a ray tracing shader
		RayTracingShaderReadAccelerationStructure,
		/// Read as any other resource in a ray tracing shader
		RayTracingShaderReadOther,

		/// Written as an acceleration structure during acceleration structure building
		AccelerationStructureBuildWrite,
		/// Read as an acceleration structure during acceleration structure building (e.g. a BLAS when building a TLAS)
		AccelerationStructureBuildRead,
		/// Written as a buffer during acceleration structure building (e.g. a staging buffer)
		AccelerationStructureBufferWrite,
	};

	bool IsCommonReadOnlyAccess(const ERenderResourceState& access);
	bool IsCommonWriteAccess(const ERenderResourceState& access);
	bool IsRasterReadOnlyAccess(const ERenderResourceState& access);
	bool IsRasterWriteAccess(const ERenderResourceState& access);
	
	class RenderResourceAccessState
	{
	public:

		RenderResourceAccessState() = default;
		RenderResourceAccessState(ERenderResourceState targetBarrier, bool bSkipSyncIfContinuous = false)
			: m_SkipSyncIfContinuous(bSkipSyncIfContinuous), m_TargetBarrier(targetBarrier)
		{
		}

		inline void SetSkipSyncIfContinuous(bool enable)
		{
			m_SkipSyncIfContinuous = enable;
		}

		inline bool GetSkipSyncIfContinuous() const
		{
			return m_SkipSyncIfContinuous;
		}

		inline ERenderResourceState GetCurrentAccess() const
		{
			return m_TargetBarrier;
		}

		inline void TransitionTo(ERenderResourceState state)
		{
			m_TargetBarrier = state;
		}

	private:

		// Skip resource synchronization between different nodes.
		bool									m_SkipSyncIfContinuous = false;
		ERenderResourceState						m_TargetBarrier = ERenderResourceState::Undefined;
	};

	struct GlobalMemoryBarrier
	{
		uint32_t								m_PrevAccessesCount = 0;
		uint32_t								m_NextAccessesCount = 0;
		const ERenderResourceState*				m_PreviousAccesses = nullptr;
		const ERenderResourceState*				m_pNextAccesses = nullptr;
	};

	class Buffer;

	struct BufferBarrier
	{
		std::span<const ERenderResourceState>	m_PrevAccesses;
		std::span<const ERenderResourceState>	m_NextAccesses;
		uint32_t								m_SrcQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		uint32_t								m_DstQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkBuffer							    m_Buffer = nullptr;
		uint32_t								m_Offset = 0;
		uint32_t								m_Size = 0;
	};

	enum class TextureMemoryLayout
	{
		/// Choose the most optimal layout for each usage. Performs layout transitions as appropriate for the access.
		Optimal,
		/// Layout accessible by all Vulkan access types on a device - no layout transitions except for presentation
		General,
		/// Similar to `General`, but also allows presentation engines to access it - no layout transitions.
		/// Requires `VK_KHR_shared_presentable_image` to be enabled, and this can only be used for shared presentable
		/// images (i.e. single-buffered swap chains).
		GeneralAndPresentation,
	};

	enum class TextureAspectFlags
	{
		Color,
		Depth,
		Stencil,
		Metadata,
	};
	
	struct TextureSubresourceRange
	{
		VkImageAspectFlags						m_AspectFlags = VK_IMAGE_ASPECT_NONE;
		uint32_t							    m_BaseMipLevel = 0u;
		uint32_t							    m_LevelCount = 1u;
		uint32_t							    m_BaseArrayLayer = 0u;
		uint32_t							    m_LayerCount = 1u;

		static TextureSubresourceRange AllSubresources(VkImageAspectFlags aspectFlags)
		{
			TextureSubresourceRange range;
			range.m_AspectFlags = aspectFlags;
			range.m_BaseMipLevel = 0u;
			range.m_BaseArrayLayer = 0u;
			range.m_LevelCount = ~0u;
			range.m_LayerCount = ~0u;
			return range;
		}
	};

	// TextureSubresourceRangeUploadRef represent a mipmap of a set of mipmap chain in a texture as subresource.
	struct TextureSubresourceRangeUploadRef
	{
		Buffer*									m_pStagingBuffer = nullptr;
		TextureAspectFlags					    m_AspectFlags = TextureAspectFlags::Color;
		uint32_t                                m_BufferBaseOffset = 0;
		uint32_t                                m_BaseMipLevel = 0;
		uint32_t                                m_Layer = 0;
		bool                                    m_UploadAllMips = true;
	};

	class Texture;

	struct TextureBarrier
	{
		bool									m_DiscardContents = true;
		std::span<const ERenderResourceState>	m_PrevAccesses;
		std::span<const ERenderResourceState>	m_NextAccesses;
		TextureMemoryLayout     				m_PrevLayout = TextureMemoryLayout::Optimal;
		TextureMemoryLayout     				m_NextLayout = TextureMemoryLayout::Optimal;
		uint32_t								m_SrcQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		uint32_t								m_DstQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkImage									m_Texture = nullptr;
		TextureSubresourceRange					m_SubresourceRange;
	};
}
