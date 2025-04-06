#include "RenderCommandList.h"

#include "PipelineState.h"
#include "RenderDevice.h"
#include "RenderResource.h"
#include "VulkanHelper.h"

namespace ZE::RenderBackend
{
	namespace
	{
		std::vector<VkMemoryBarrier> sTempMemoryBarriers;
		std::vector<VkBufferMemoryBarrier> sTempBufferBarriers;
		std::vector<VkImageMemoryBarrier> sTempTextureBarriers;
		
		bool IsWriteAccess(ERenderResourceState state)
		{
			switch (state)
			{
				case ERenderResourceState::VertexShaderWrite:
				case ERenderResourceState::TessellationControlShaderWrite:
				case ERenderResourceState::TessellationEvaluationShaderWrite:
				case ERenderResourceState::GeometryShaderWrite:
				case ERenderResourceState::FragmentShaderWrite:
				case ERenderResourceState::ColorAttachmentWrite:
				case ERenderResourceState::DepthStencilAttachmentWrite:
				case ERenderResourceState::DepthAttachmentWriteStencilReadOnly:
				case ERenderResourceState::StencilAttachmentWriteDepthReadOnly:
				case ERenderResourceState::ComputeShaderWrite:
				case ERenderResourceState::AnyShaderWrite:
				case ERenderResourceState::TransferWrite:
				case ERenderResourceState::HostWrite:
				
				case ERenderResourceState::General:
				case ERenderResourceState::ColorAttachmentReadWrite:

				return true;
				
				default:
					return false;
			}
			return false;
		}
		
		PipelineResourceAccessInfo GetPipelineResourceAccessInfo(ERenderResourceState state)
        {
            switch (state)
            {
                case ERenderResourceState::Undefined:
					return PipelineResourceAccessInfo{ .m_StageMask = 0, .m_AccessMask = 0, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::IndirectBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, .m_AccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::VertexBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, .m_AccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::IndexBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, .m_AccessMask = VK_ACCESS_INDEX_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };

                case ERenderResourceState::VertexShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::VertexShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::VertexShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::TessellationControlShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::TessellationControlShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::TessellationEvaluationShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::TessellationEvaluationShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::GeometryShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::GeometryShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::FragmentShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                // QA
                case ERenderResourceState::FragmentShaderReadColorInputAttachment:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                // QA
                case ERenderResourceState::FragmentShaderReadDepthStencilInputAttachment:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                case ERenderResourceState::FragmentShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::ColorAttachmentRead:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .m_AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case ERenderResourceState::DepthStencilAttachmentRead:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, .m_AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

                case ERenderResourceState::ComputeShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::ComputeShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::ComputeShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::AnyShaderReadUniformBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::AnyShaderReadUniformBufferOrVertexBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::AnyShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::AnyShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::TransferRead:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TRANSFER_BIT, .m_AccessMask = VK_ACCESS_TRANSFER_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
                case ERenderResourceState::HostRead:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_HOST_BIT, .m_AccessMask = VK_ACCESS_HOST_READ_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::Present:
					return PipelineResourceAccessInfo{ .m_StageMask = 0, .m_AccessMask = 0, .m_ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

                case ERenderResourceState::VertexShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::TessellationControlShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::TessellationEvaluationShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::GeometryShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::FragmentShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::ColorAttachmentWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .m_AccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case ERenderResourceState::DepthStencilAttachmentWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, .m_AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                case ERenderResourceState::DepthAttachmentWriteStencilReadOnly:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, .m_AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL };
                case ERenderResourceState::StencilAttachmentWriteDepthReadOnly:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, .m_AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL };

                case ERenderResourceState::ComputeShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::AnyShaderWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_SHADER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };
                case ERenderResourceState::TransferWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_TRANSFER_BIT, .m_AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
                case ERenderResourceState::HostWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_HOST_BIT, .m_AccessMask = VK_ACCESS_HOST_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::ColorAttachmentReadWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .m_AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case ERenderResourceState::General:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .m_AccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::RayTracingShaderReadSampledImageOrUniformTexelBuffer:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout =VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::RayTracingShaderReadColorInputAttachment:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .m_AccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, .m_ImageLayout =VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case ERenderResourceState::RayTracingShaderReadDepthStencilInputAttachment:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .m_AccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, .m_ImageLayout =VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                case ERenderResourceState::RayTracingShaderReadAccelerationStructure:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .m_AccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, .m_ImageLayout =VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::RayTracingShaderReadOther:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .m_AccessMask = VK_ACCESS_SHADER_READ_BIT, .m_ImageLayout =VK_IMAGE_LAYOUT_GENERAL };

                case ERenderResourceState::AccelerationStructureBuildWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .m_AccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, .m_ImageLayout =VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::AccelerationStructureBuildRead:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .m_AccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, .m_ImageLayout =VK_IMAGE_LAYOUT_UNDEFINED };
                case ERenderResourceState::AccelerationStructureBufferWrite:
					return PipelineResourceAccessInfo{ .m_StageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .m_AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED };

                default:
                {
                	ZE_CHECK(false);
                	return {};
                }
            }
			return {};
        }

		VkAttachmentLoadOp ToVkLoadOp(ERenderTargetLoadOperation loadOp)
		{
			switch ( loadOp )
			{
				case ERenderTargetLoadOperation::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
				case ERenderTargetLoadOperation::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
				case ERenderTargetLoadOperation::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			ZE_CHECK(false);
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		}

		VkAttachmentStoreOp ToVkStoreOp(ERenderTargetStoreOperation storeOp)
		{
			switch ( storeOp )
			{
				case ERenderTargetStoreOperation::Store: return VK_ATTACHMENT_STORE_OP_STORE;
				case ERenderTargetStoreOperation::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
			ZE_CHECK(false);
			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		}
	}
	
	GlobalMemoryBarrierTransition GetMemoryBarrierTransition(const GlobalMemoryBarrier& globalBarrier)
	{
		GlobalMemoryBarrierTransition barrier;
        barrier.m_SrcStage = 0;
        barrier.m_DstStage = 0;
        barrier.m_Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.m_Barrier.pNext = nullptr;
        barrier.m_Barrier.srcAccessMask = 0;
        barrier.m_Barrier.dstAccessMask = 0;

        for (uint32_t i = 0; i < globalBarrier.m_PrevAccessesCount; ++i)
        {
            auto const& prevAccess = globalBarrier.m_PreviousAccesses[i];
            auto accessInfo = GetPipelineResourceAccessInfo( prevAccess );

            // what stage this resource is used in previous stage
            barrier.m_SrcStage |= accessInfo.m_StageMask;

            // only access the write access
            if (IsWriteAccess(prevAccess))
            {
                barrier.m_Barrier.srcAccessMask |= accessInfo.m_AccessMask;
            }
        }

        for (uint32_t i = 0; i < globalBarrier.m_NextAccessesCount; ++i)
        {
            auto const& nextAccess = globalBarrier.m_pNextAccesses[i];
            auto accessInfo = GetPipelineResourceAccessInfo( nextAccess );

            // what stage this resource is used in previous stage
            barrier.m_DstStage |= accessInfo.m_StageMask;

            // if write access happened before, it must be visible to the dst access.
            // (i.e. RAW (Read-After-Write) operation or WAW (Write-After-Write) )
            if (barrier.m_Barrier.srcAccessMask != 0)
            {
                barrier.m_Barrier.dstAccessMask |= accessInfo.m_AccessMask;
            }
        }

        // ensure that the stage masks are valid if no stages were determined
        if (barrier.m_SrcStage == 0)
        {
            barrier.m_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if (barrier.m_DstStage == 0)
        {
            barrier.m_DstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        return barrier;
	}
	
	BufferBarrierTransition GetBufferBarrierTransition(const BufferBarrier& bufferBarrier)
	{
        ZE_CHECK(bufferBarrier.m_Buffer);

        BufferBarrierTransition barrier;
        barrier.m_SrcStage = 0;
        barrier.m_DstStage = 0;
        barrier.m_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.m_Barrier.pNext = nullptr;
        barrier.m_Barrier.srcAccessMask = 0;
        barrier.m_Barrier.dstAccessMask = 0;
        barrier.m_Barrier.srcQueueFamilyIndex = bufferBarrier.m_SrcQueueFamilyIndex;
        barrier.m_Barrier.dstQueueFamilyIndex = bufferBarrier.m_DstQueueFamilyIndex;
        barrier.m_Barrier.buffer = bufferBarrier.m_Buffer;
        barrier.m_Barrier.offset = static_cast<VkDeviceSize>(bufferBarrier.m_Offset);
        barrier.m_Barrier.size = static_cast<VkDeviceSize>(bufferBarrier.m_Size);

        for (auto const& prevAccess : bufferBarrier.m_PrevAccesses)
        {
            auto accessInfo = GetPipelineResourceAccessInfo(prevAccess);

            // what stage this resource is used in previous stage
            barrier.m_SrcStage |= accessInfo.m_StageMask;

            // only access the write access
            if (IsWriteAccess(prevAccess))
            {
                barrier.m_Barrier.srcAccessMask |= accessInfo.m_AccessMask;
            }
        }

        for (auto const& nextAccess : bufferBarrier.m_NextAccesses)
        {
            auto accessInfo = GetPipelineResourceAccessInfo(nextAccess);

            // what stage this resource is used in previous stage
            barrier.m_DstStage |= accessInfo.m_StageMask;

            // if write access happened before, it must be visible to the dst access.
            // (i.e. RAW (Read-After-Write) operation or WAW (Write-After-Write) )
			if (barrier.m_Barrier.srcAccessMask != 0)
            {
                barrier.m_Barrier.dstAccessMask |= accessInfo.m_AccessMask;
            }
        }

        // ensure that the stage masks are valid if no stages were determined
        if (barrier.m_SrcStage == 0)
        {
            barrier.m_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if (barrier.m_DstStage == 0)
        {
            barrier.m_DstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        return barrier;
	}
	
	TextureBarrierTransition GetTextureBarrierTransition(const TextureBarrier& textureBarrier)
	{
		ZE_CHECK(textureBarrier.m_Texture);

		TextureBarrierTransition barrier = {};
        barrier.m_SrcStage = 0;
        barrier.m_DstStage = 0;
        barrier.m_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.m_Barrier.pNext = nullptr;
        barrier.m_Barrier.srcAccessMask = 0;
        barrier.m_Barrier.dstAccessMask = 0;
        barrier.m_Barrier.srcQueueFamilyIndex = textureBarrier.m_SrcQueueFamilyIndex;
        barrier.m_Barrier.dstQueueFamilyIndex = textureBarrier.m_DstQueueFamilyIndex;
        barrier.m_Barrier.image = textureBarrier.m_Texture;

        barrier.m_Barrier.subresourceRange.aspectMask = textureBarrier.m_SubresourceRange.m_AspectFlags;
        barrier.m_Barrier.subresourceRange.baseMipLevel = textureBarrier.m_SubresourceRange.m_BaseMipLevel;
        barrier.m_Barrier.subresourceRange.levelCount = textureBarrier.m_SubresourceRange.m_LevelCount;
        barrier.m_Barrier.subresourceRange.baseArrayLayer = textureBarrier.m_SubresourceRange.m_BaseArrayLayer;
        barrier.m_Barrier.subresourceRange.layerCount = textureBarrier.m_SubresourceRange.m_LayerCount;

        for (auto const& prevAccess : textureBarrier.m_PrevAccesses)
        {
            auto accessInfo = GetPipelineResourceAccessInfo(prevAccess);

            // what stage this resource is used in previous stage
            barrier.m_SrcStage |= accessInfo.m_StageMask;

            // only access the write access
            if (IsWriteAccess(prevAccess))
            {
                barrier.m_Barrier.srcAccessMask |= accessInfo.m_AccessMask;
            }

            if (textureBarrier.m_DiscardContents)
            {
                // we don't care about the previous image layout
                barrier.m_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            else
            {
                switch (textureBarrier.m_PrevLayout)
                {
                    case TextureMemoryLayout::Optimal:
						barrier.m_Barrier.newLayout = accessInfo.m_ImageLayout;
                    break;
                    case TextureMemoryLayout::General:
                    {
                        if (prevAccess == ERenderResourceState::Present)
                        {
                            barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        }
                        else
                        {
                            barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        }
                    }
                    break;
					case TextureMemoryLayout::GeneralAndPresentation:
					{
						ZE_CHECK(false);
						barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					}
                	break;
                }
            }
        }

        for (auto const& nextAccess : textureBarrier.m_NextAccesses)
        {
            auto accessInfo = GetPipelineResourceAccessInfo(nextAccess);

            // what stage this resource is used in previous stage
            barrier.m_DstStage |= accessInfo.m_StageMask;

            // if write access happened before, it must be visible to the dst access.
        	// (i.e. RAW (Read-After-Write) operation or WAW (Write-After-Write) )
            if (barrier.m_Barrier.srcAccessMask != 0)
            {
                barrier.m_Barrier.dstAccessMask |= accessInfo.m_AccessMask;
            }

            switch (textureBarrier.m_NextLayout)
            {
                case TextureMemoryLayout::Optimal:
					barrier.m_Barrier.newLayout = accessInfo.m_ImageLayout;
                break;
                case TextureMemoryLayout::General:
                {
                    if (nextAccess == ERenderResourceState::Present)
                    {
                        barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    }
                    else
                    {
                        barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    }
                }
                break;
                case TextureMemoryLayout::GeneralAndPresentation:
                {
                	ZE_CHECK(false);
                	barrier.m_Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                }
            	break;
            }
        }

        // ensure that the stage masks are valid if no stages were determined
        if (barrier.m_SrcStage == 0)
        {
            barrier.m_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if (barrier.m_DstStage == 0)
        {
            barrier.m_DstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        return barrier;
	}
	
	RenderCommandList::RenderCommandList(RenderDevice& renderDevice)
	{
		SetRenderDevice(&renderDevice);
		
		VulkanZeroStruct(VkCommandPoolCreateInfo, commandPoolCI);
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		// TODO: sepecify command list type
		commandPoolCI.queueFamilyIndex = GetRenderDevice().m_GraphicQueueFamilyIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		m_QueueIndex = commandPoolCI.queueFamilyIndex;

		VulkanCheckSucceed(vkCreateCommandPool(GetRenderDevice().GetNativeDevice(), &commandPoolCI, nullptr, &m_CommandPool));
		
		VulkanZeroStruct(VkCommandBufferAllocateInfo, allocateInfo);
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(GetRenderDevice().GetNativeDevice(), &allocateInfo, &m_CommandBuffer);

		sTempMemoryBarriers.reserve(1);
		sTempBufferBarriers.reserve(16);
		sTempTextureBarriers.reserve(16);
	}

	RenderCommandList::~RenderCommandList()
	{
		vkDestroyCommandPool(GetRenderDevice().GetNativeDevice(), m_CommandPool, nullptr);
		m_CommandPool = nullptr;
	}

	bool RenderCommandList::BeginRecord()
	{
		ZE_CHECK(!m_IsCommandRecording);
		VulkanZeroStruct(VkCommandBufferBeginInfo, beginInfo);
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VulkanCheckSucceed(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

		m_IsCommandRecording = true;
		return true;
	}

	void RenderCommandList::EndRecord()
	{
		ZE_CHECK(m_IsCommandRecording);
		VulkanCheckSucceed(vkEndCommandBuffer(m_CommandBuffer));
		m_IsCommandRecording = false;
	}
	
	void RenderCommandList::Reset() const
	{
		VulkanCheckSucceed(vkResetCommandBuffer(m_CommandBuffer, 0));
	}

	void RenderCommandList::CmdBeginDynamicRendering(const glm::uvec2& viewportSize, std::span<Texture*> renderTargets, std::span<RenderPassRenderTargetBinding> colorBindings) const
	{
		ZE_CHECK(m_IsCommandRecording);
		ZE_CHECK(renderTargets.size() == colorBindings.size());
		
		if (renderTargets.empty())
		{
			return;
		}
		
		std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
		VkRenderingAttachmentInfo depthStencilAttachmentInfo;
		colorAttachmentInfos.reserve(colorBindings.size());

		bool bIsDepthStencil = false;
		for (uint32_t i = 0; i < colorBindings.size(); ++i)
		{
			const auto& attachment = colorBindings[i];
			auto* pRenderTarget = renderTargets[i];
			
			VulkanZeroStruct(VkRenderingAttachmentInfo, attachmentInfo);
			attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachmentInfo.imageView = pRenderTarget->GetOrCreateView();
			attachmentInfo.imageLayout = SpeculateVkImageLayoutFromDesc(pRenderTarget->GetDesc());
			attachmentInfo.loadOp = ToVkLoadOp(attachment.m_LoadOp);
			attachmentInfo.storeOp = ToVkStoreOp(attachment.m_StoreOp);
			
			if (std::holds_alternative<glm::vec4>(attachment.m_ClearValue))
			{
				const glm::vec4& clearColor = std::get<glm::vec4>(attachment.m_ClearValue);
				attachmentInfo.clearValue.color.float32[0] = clearColor.x;
				attachmentInfo.clearValue.color.float32[1] = clearColor.y;
				attachmentInfo.clearValue.color.float32[2] = clearColor.z;
				attachmentInfo.clearValue.color.float32[3] = clearColor.w;
			}
			else if (std::holds_alternative<DepthStencilClearValue>(attachment.m_ClearValue))
			{
				ZE_CHECK(!bIsDepthStencil);
				bIsDepthStencil = true;
				const DepthStencilClearValue& depthClearValue = std::get<DepthStencilClearValue>(attachment.m_ClearValue);
				attachmentInfo.clearValue.depthStencil.depth = depthClearValue.m_ClearDepth;
				attachmentInfo.clearValue.depthStencil.stencil = static_cast<uint32_t>(depthClearValue.m_ClearStencil);
			}

			if (bIsDepthStencil)
			{
				depthStencilAttachmentInfo = attachmentInfo;
			}
			else
			{
				colorAttachmentInfos.push_back(attachmentInfo);
			}
		}
		
		VulkanZeroStruct(VkRenderingInfo, renderingInfo);
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
		renderingInfo.pColorAttachments = colorAttachmentInfos.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachmentInfo;
		renderingInfo.pStencilAttachment = &depthStencilAttachmentInfo;
		renderingInfo.renderArea.offset = {0, 0};
		renderingInfo.renderArea.extent = {viewportSize.x, viewportSize.y};
		
		vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);
	}

	void RenderCommandList::CmdEndDynamicRendering() const
	{
		ZE_CHECK(m_IsCommandRecording);

		vkCmdEndRendering(m_CommandBuffer);
	}
	
	void RenderCommandList::CmdSetViewport(const glm::uvec2& viewportSize) const
	{
		ZE_CHECK(m_IsCommandRecording);

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(viewportSize.x);
		viewport.height = static_cast<float>(viewportSize.y);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}
	
	void RenderCommandList::CmdSetScissor(const glm::uvec2& viewportSize) const
	{
		ZE_CHECK(m_IsCommandRecording);

		VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = viewportSize.x;
		scissor.extent.height = viewportSize.y;
		
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void RenderCommandList::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
	{
		ZE_CHECK(m_IsCommandRecording);

		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
	
	void RenderCommandList::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
	{
		ZE_CHECK(m_IsCommandRecording);

		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	// pipeline resource commands
	void RenderCommandList::CmdBindPipeline(VkPipelineBindPoint bindPoint, PipelineState* pPipelineState) const
	{
		ZE_CHECK(m_IsCommandRecording);
		ZE_CHECK(pPipelineState);
		
		vkCmdBindPipeline(m_CommandBuffer, bindPoint, pPipelineState->m_Pipeline);
	}
	
	void RenderCommandList::CmdBindShaderResource(VkPipelineBindPoint bindPoint, PipelineState* pPipelineState, const std::vector<VkDescriptorSet>& sets) const
	{
		ZE_CHECK(m_IsCommandRecording);

		vkCmdBindDescriptorSets(m_CommandBuffer, bindPoint, pPipelineState->m_Layout,
			0, static_cast<uint32_t>(sets.size()), sets.data(),
			0, nullptr);
	}
	
	void RenderCommandList::CmdBindVertexInput(const Buffer* pVertexBuffer, const Buffer* pIndexBuffer) const
	{
		ZE_CHECK(m_IsCommandRecording);
		ZE_CHECK(pVertexBuffer);

		// TODO: support vertex batch
		const VkBuffer vertexBuffers[1] = { pVertexBuffer->GetNativeHandle() };
		constexpr VkDeviceSize offsets[1] = { 0ull };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, vertexBuffers, offsets);
		if (pIndexBuffer)
		{
			// TODO: uint16 index type
			vkCmdBindIndexBuffer(m_CommandBuffer, pIndexBuffer->GetNativeHandle(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	// barrier commands
	void RenderCommandList::CmdResourceBarrier(const GlobalMemoryBarrier* pMemoryBarrier, std::span<const BufferBarrier> pBufferBarriers, std::span<const TextureBarrier> pTextureBarriers) const
	{
		ZE_CHECK(m_IsCommandRecording);
		
		VkPipelineStageFlags srcStageFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags dstStageFlag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		if (pMemoryBarrier)
		{
			auto transition = GetMemoryBarrierTransition(*pMemoryBarrier);
			srcStageFlag |= transition.m_SrcStage;
			dstStageFlag |= transition.m_DstStage;
			sTempMemoryBarriers.push_back(transition.m_Barrier);
		}

		for (auto& bufferBarrier : pBufferBarriers)
		{
			auto transition = GetBufferBarrierTransition(bufferBarrier);
			srcStageFlag |= transition.m_SrcStage;
			dstStageFlag |= transition.m_DstStage;
			sTempBufferBarriers.push_back(transition.m_Barrier);
		}

		for (auto& textureBarrier : pTextureBarriers)
		{
			auto transition = GetTextureBarrierTransition(textureBarrier);
			srcStageFlag |= transition.m_SrcStage;
			dstStageFlag |= transition.m_DstStage;
			sTempTextureBarriers.push_back(transition.m_Barrier);
		}

		vkCmdPipelineBarrier(
			m_CommandBuffer, srcStageFlag, dstStageFlag, 0,
			static_cast<uint32_t>(sTempMemoryBarriers.size()), sTempMemoryBarriers.data(),
			static_cast<uint32_t>(sTempBufferBarriers.size()), sTempBufferBarriers.data(),
			static_cast<uint32_t>(sTempTextureBarriers.size()), sTempTextureBarriers.data()
			);

		sTempMemoryBarriers.clear();
		sTempBufferBarriers.clear();
		sTempTextureBarriers.clear();
	}

	// transfer commands
	void RenderCommandList::CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t srcOffset) const
	{
		ZE_CHECK(m_IsCommandRecording);

		VkBufferCopy bufferCopy;
		bufferCopy.size = pDstBuffer->GetDesc().m_Size;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.dstOffset = 0;

		vkCmdCopyBuffer(m_CommandBuffer, pSrcBuffer->GetNativeHandle(), pDstBuffer->GetNativeHandle(), 1, &bufferCopy);
	}
}