#include "RenderResourceState.h"

namespace ZE::RenderBackend
{
	bool IsCommonReadOnlyAccess(const RenderResourceState& access)
	{
		switch (access)
		{
		case RenderResourceState::IndirectBuffer:
		case RenderResourceState::VertexBuffer:
		case RenderResourceState::IndexBuffer:
		case RenderResourceState::VertexShaderReadUniformBuffer:
		case RenderResourceState::VertexShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::VertexShaderReadOther:
		case RenderResourceState::TessellationControlShaderReadUniformBuffer:
		case RenderResourceState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::TessellationControlShaderReadOther:
		case RenderResourceState::TessellationEvaluationShaderReadUniformBuffer:
		case RenderResourceState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::TessellationEvaluationShaderReadOther:
		case RenderResourceState::GeometryShaderReadUniformBuffer:
		case RenderResourceState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::GeometryShaderReadOther:
		case RenderResourceState::FragmentShaderReadUniformBuffer:
		case RenderResourceState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::FragmentShaderReadColorInputAttachment:
		case RenderResourceState::FragmentShaderReadDepthStencilInputAttachment:
		case RenderResourceState::FragmentShaderReadOther:
		case RenderResourceState::ColorAttachmentRead:
		case RenderResourceState::DepthStencilAttachmentRead:
		case RenderResourceState::ComputeShaderReadUniformBuffer:
		case RenderResourceState::ComputeShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::ComputeShaderReadOther:
		case RenderResourceState::AnyShaderReadUniformBuffer:
		case RenderResourceState::AnyShaderReadUniformBufferOrVertexBuffer:
		case RenderResourceState::AnyShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::AnyShaderReadOther:
		case RenderResourceState::TransferRead:
		case RenderResourceState::HostRead:
		case RenderResourceState::Present:

		case RenderResourceState::RayTracingShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::RayTracingShaderReadColorInputAttachment:
		case RenderResourceState::RayTracingShaderReadDepthStencilInputAttachment:
		case RenderResourceState::RayTracingShaderReadAccelerationStructure:
		case RenderResourceState::RayTracingShaderReadOther:

		case RenderResourceState::AccelerationStructureBuildRead:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsCommonWriteAccess(const RenderResourceState& access)
	{
		switch (access)
		{
		case RenderResourceState::VertexShaderWrite:
		case RenderResourceState::TessellationControlShaderWrite:
		case RenderResourceState::TessellationEvaluationShaderWrite:
		case RenderResourceState::GeometryShaderWrite:
		case RenderResourceState::FragmentShaderWrite:
		case RenderResourceState::ColorAttachmentWrite:
		case RenderResourceState::DepthStencilAttachmentWrite:
		case RenderResourceState::DepthAttachmentWriteStencilReadOnly:
		case RenderResourceState::StencilAttachmentWriteDepthReadOnly:
		case RenderResourceState::ComputeShaderWrite:
		case RenderResourceState::AnyShaderWrite:
		case RenderResourceState::TransferWrite:
		case RenderResourceState::HostWrite:

			// TODO: Should we put General in write access?
		case RenderResourceState::General:

		case RenderResourceState::ColorAttachmentReadWrite:

		case RenderResourceState::AccelerationStructureBuildWrite:
		case RenderResourceState::AccelerationStructureBufferWrite:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsRasterReadOnlyAccess(const RenderResourceState& access)
	{
		switch (access)
		{
		case RenderResourceState::VertexBuffer:
		case RenderResourceState::IndexBuffer:

		case RenderResourceState::VertexShaderReadUniformBuffer:
		case RenderResourceState::VertexShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::VertexShaderReadOther:
		case RenderResourceState::TessellationControlShaderReadUniformBuffer:
		case RenderResourceState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::TessellationControlShaderReadOther:
		case RenderResourceState::TessellationEvaluationShaderReadUniformBuffer:
		case RenderResourceState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::TessellationEvaluationShaderReadOther:
		case RenderResourceState::GeometryShaderReadUniformBuffer:
		case RenderResourceState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::GeometryShaderReadOther:
		case RenderResourceState::FragmentShaderReadUniformBuffer:
		case RenderResourceState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::FragmentShaderReadColorInputAttachment:
		case RenderResourceState::FragmentShaderReadDepthStencilInputAttachment:
		case RenderResourceState::FragmentShaderReadOther:
		case RenderResourceState::ColorAttachmentRead:
		case RenderResourceState::DepthStencilAttachmentRead:

		case RenderResourceState::DepthAttachmentWriteStencilReadOnly:
		case RenderResourceState::StencilAttachmentWriteDepthReadOnly:
		case RenderResourceState::ColorAttachmentReadWrite:

		case RenderResourceState::AnyShaderReadUniformBuffer:
		case RenderResourceState::AnyShaderReadUniformBufferOrVertexBuffer:
		case RenderResourceState::AnyShaderReadSampledImageOrUniformTexelBuffer:
		case RenderResourceState::AnyShaderReadOther:

			return true;
			break;

		default:
			return false;
			break;
		}
	}

	bool IsRasterWriteAccess(const RenderResourceState& access)
	{
		switch (access)
		{
		case RenderResourceState::ColorAttachmentReadWrite:

		case RenderResourceState::VertexShaderWrite:
		case RenderResourceState::TessellationControlShaderWrite:
		case RenderResourceState::TessellationEvaluationShaderWrite:
		case RenderResourceState::GeometryShaderWrite:
		case RenderResourceState::FragmentShaderWrite:
		case RenderResourceState::ColorAttachmentWrite:
		case RenderResourceState::DepthStencilAttachmentWrite:
		case RenderResourceState::DepthAttachmentWriteStencilReadOnly:
		case RenderResourceState::StencilAttachmentWriteDepthReadOnly:

		case RenderResourceState::AnyShaderWrite:


			return true;
			break;

		default:
			return false;
			break;
		}
	}


}

