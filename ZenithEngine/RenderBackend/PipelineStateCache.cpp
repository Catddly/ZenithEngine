#include "PipelineStateCache.h"

#include <ranges>

#include "RenderDevice.h"

namespace ZE::RenderBackend
{
	PipelineStateCache::PipelineStateCache(RenderDevice& renderDevice)
	{
		SetRenderDevice(&renderDevice);
	}
	
	PipelineStateCache::~PipelineStateCache()
	{
		m_Cache.clear();
	}

	std::shared_ptr<PipelineState> PipelineStateCache::CreateGraphicPipelineState(const GraphicPipelineStateCreateDesc& CreateDesc)
	{
		const uint64_t hash = CreateDesc.GetHash();
		if (const auto iter = m_Cache.find(hash); iter != m_Cache.end())
		{
			return iter->second;
		}
		
		auto pGraphicPSO = std::shared_ptr<GraphicPipelineState>(GraphicPipelineState::Create(GetRenderDevice(), CreateDesc));
		if (pGraphicPSO)
		{
			m_Cache.emplace(hash, pGraphicPSO);
		}
		return pGraphicPSO;
	}
}
