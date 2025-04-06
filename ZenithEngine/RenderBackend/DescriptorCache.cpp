#include "DescriptorCache.h"

#include "PipelineState.h"
#include "RenderDevice.h"
#include "VulkanHelper.h"

#include <ranges>

namespace ZE::RenderBackend
{
	DescriptorCache::DescriptorCache(RenderDevice& renderDevice)
	{
		SetRenderDevice(&renderDevice);
	}
	
	std::vector<VkDescriptorSet>& DescriptorCache::FindOrAdd(PipelineState* pPipelineState)
	{
		if (!pPipelineState)
		{
			static std::vector<VkDescriptorSet> dummy{};
			return dummy;
		}
		
		auto iter = m_PipelineDescriptorCache.find(pPipelineState);
		if (iter == m_PipelineDescriptorCache.end())
		{
			auto sets = std::vector<VkDescriptorSet>{};
			
			sets.resize(pPipelineState->m_DescriptorSetLayouts.size());

			for (uint32_t setIndex = 0; setIndex < sets.size(); ++setIndex)
			{
				VulkanZeroStruct(VkDescriptorSetAllocateInfo, descriptorAllocInfo);
				descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorAllocInfo.descriptorSetCount = 1u;
				descriptorAllocInfo.descriptorPool = pPipelineState->m_DescriptorPool;
				descriptorAllocInfo.pSetLayouts = &pPipelineState->m_DescriptorSetLayouts[setIndex];
				
				VulkanCheckSucceed(vkAllocateDescriptorSets(GetRenderDevice().GetNativeDevice(), &descriptorAllocInfo, &sets[setIndex]));
			}

			auto insertIter = m_PipelineDescriptorCache.emplace(pPipelineState, std::move(sets));
			return insertIter.first->second;
		}

		return iter->second;
	}
	
	void DescriptorCache::MarkDirty(PipelineState* pPipelineState)
	{
		if (const auto iter = m_PipelineDescriptorCache.find(pPipelineState); iter != m_PipelineDescriptorCache.end())
		{
			m_PipelineDescriptorCache.erase(iter);
		}
	}

	void DescriptorCache::FreeAll()
	{
		for (auto& [pPSO, set] : m_PipelineDescriptorCache)
		{
			VulkanCheckSucceed(vkFreeDescriptorSets(GetRenderDevice().GetNativeDevice(), pPSO->m_DescriptorPool, static_cast<uint32_t>(set.size()), set.data()));
		}
		m_PipelineDescriptorCache.clear();
	}
}
