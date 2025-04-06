#pragma once

#include "RenderDeviceChild.h"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <unordered_map>

namespace ZE::RenderBackend
{
	class RenderDevice;
	class PipelineState;

	class DescriptorCache : public RenderDeviceChild
	{
	public:

		DescriptorCache(RenderDevice& renderDevice);

		std::vector<VkDescriptorSet>& FindOrAdd(PipelineState* pPipelineState);
		void MarkDirty(PipelineState* pPipelineState);

	private:
		
		void FreeAll();
		
	private:
		
		std::unordered_map<PipelineState*, std::vector<VkDescriptorSet>>		m_PipelineDescriptorCache;
	};
}
