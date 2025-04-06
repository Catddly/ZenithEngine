#include "RenderResourceState.h"

namespace ZE::RenderBackend
{
	bool IsCommonReadOnlyAccess(const ERenderResourceState& access)
	{
		switch (access)
		{
		case ERenderResourceState::IndirectBuffer:
		case ERenderResourceState::VertexBuffer:
		case ERenderResourceState::IndexBuffer:
		case ERenderResourceState::VertexShaderReadUniformBuffer:
		case ERenderResourceState::VertexShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::VertexShaderReadOther:
		case ERenderResourceState::TessellationControlShaderReadUniformBuffer:
		case ERenderResourceState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::TessellationControlShaderReadOther:
		case ERenderResourceState::TessellationEvaluationShaderReadUniformBuffer:
		case ERenderResourceState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::TessellationEvaluationShaderReadOther:
		case ERenderResourceState::GeometryShaderReadUniformBuffer:
		case ERenderResourceState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::GeometryShaderReadOther:
		case ERenderResourceState::FragmentShaderReadUniformBuffer:
		case ERenderResourceState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::FragmentShaderReadColorInputAttachment:
		case ERenderResourceState::FragmentShaderReadDepthStencilInputAttachment:
		case ERenderResourceState::FragmentShaderReadOther:
		case ERenderResourceState::ColorAttachmentRead:
		case ERenderResourceState::DepthStencilAttachmentRead:
		case ERenderResourceState::ComputeShaderReadUniformBuffer:
		case ERenderResourceState::ComputeShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::ComputeShaderReadOther:
		case ERenderResourceState::AnyShaderReadUniformBuffer:
		case ERenderResourceState::AnyShaderReadUniformBufferOrVertexBuffer:
		case ERenderResourceState::AnyShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::AnyShaderReadOther:
		case ERenderResourceState::TransferRead:
		case ERenderResourceState::HostRead:
		case ERenderResourceState::Present:

		case ERenderResourceState::RayTracingShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::RayTracingShaderReadColorInputAttachment:
		case ERenderResourceState::RayTracingShaderReadDepthStencilInputAttachment:
		case ERenderResourceState::RayTracingShaderReadAccelerationStructure:
		case ERenderResourceState::RayTracingShaderReadOther:

		case ERenderResourceState::AccelerationStructureBuildRead:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsCommonWriteAccess(const ERenderResourceState& access)
	{
		switch (access)
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

			// TODO: Should we put General in write access?
		case ERenderResourceState::General:

		case ERenderResourceState::ColorAttachmentReadWrite:

		case ERenderResourceState::AccelerationStructureBuildWrite:
		case ERenderResourceState::AccelerationStructureBufferWrite:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsRasterReadOnlyAccess(const ERenderResourceState& access)
	{
		switch (access)
		{
		case ERenderResourceState::VertexBuffer:
		case ERenderResourceState::IndexBuffer:

		case ERenderResourceState::VertexShaderReadUniformBuffer:
		case ERenderResourceState::VertexShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::VertexShaderReadOther:
		case ERenderResourceState::TessellationControlShaderReadUniformBuffer:
		case ERenderResourceState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::TessellationControlShaderReadOther:
		case ERenderResourceState::TessellationEvaluationShaderReadUniformBuffer:
		case ERenderResourceState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::TessellationEvaluationShaderReadOther:
		case ERenderResourceState::GeometryShaderReadUniformBuffer:
		case ERenderResourceState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::GeometryShaderReadOther:
		case ERenderResourceState::FragmentShaderReadUniformBuffer:
		case ERenderResourceState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::FragmentShaderReadColorInputAttachment:
		case ERenderResourceState::FragmentShaderReadDepthStencilInputAttachment:
		case ERenderResourceState::FragmentShaderReadOther:
		case ERenderResourceState::ColorAttachmentRead:
		case ERenderResourceState::DepthStencilAttachmentRead:

		case ERenderResourceState::DepthAttachmentWriteStencilReadOnly:
		case ERenderResourceState::StencilAttachmentWriteDepthReadOnly:
		case ERenderResourceState::ColorAttachmentReadWrite:

		case ERenderResourceState::AnyShaderReadUniformBuffer:
		case ERenderResourceState::AnyShaderReadUniformBufferOrVertexBuffer:
		case ERenderResourceState::AnyShaderReadSampledImageOrUniformTexelBuffer:
		case ERenderResourceState::AnyShaderReadOther:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsRasterWriteAccess(const ERenderResourceState& access)
	{
		switch (access)
		{
		case ERenderResourceState::ColorAttachmentReadWrite:

		case ERenderResourceState::VertexShaderWrite:
		case ERenderResourceState::TessellationControlShaderWrite:
		case ERenderResourceState::TessellationEvaluationShaderWrite:
		case ERenderResourceState::GeometryShaderWrite:
		case ERenderResourceState::FragmentShaderWrite:
		case ERenderResourceState::ColorAttachmentWrite:
		case ERenderResourceState::DepthStencilAttachmentWrite:
		case ERenderResourceState::DepthAttachmentWriteStencilReadOnly:
		case ERenderResourceState::StencilAttachmentWriteDepthReadOnly:

		case ERenderResourceState::AnyShaderWrite:
			
			return true;
			break;

		default:
			return false;
			break;
		}
	}
}

