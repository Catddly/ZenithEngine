#pragma once

#include "PipelineState.h"

#include <memory>
#include <unordered_map>

namespace ZE::RenderBackend
{
	class RenderDevice;
	class PipelineState;
	
	class VertexShader;
	class PixelShader;
	
	class PipelineStateCache final : public RenderDeviceChild
	{
	public:

		PipelineStateCache(RenderDevice& renderDevice);
		~PipelineStateCache();
		
		std::shared_ptr<PipelineState> CreateGraphicPipelineState(const GraphicPipelineStateCreateDesc& CreateDesc);
		
	private:

		std::unordered_map<uint64_t, std::shared_ptr<PipelineState>>			m_Cache;
	};
}
