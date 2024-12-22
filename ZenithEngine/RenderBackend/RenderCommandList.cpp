#include "RenderCommandList.h"

#include "RenderDevice.h"
#include "VulkanHelper.h"

namespace ZE::RenderBackend
{
	RenderCommandList::RenderCommandList(RenderDevice& renderDevice)
		: m_renderDevice(renderDevice)
	{
		VulkanZeroStruct(VkCommandPoolCreateInfo, commandPoolCI);
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		// TODO: sepecify command list type
		commandPoolCI.queueFamilyIndex = m_renderDevice.m_GraphicQueueFamilyIndex;

		VulkanCheckSucceed(vkCreateCommandPool(m_renderDevice.m_Device, &commandPoolCI, nullptr, &m_CommandPool));
		
		for (auto& commandBuffer : m_CommandBufferArray)
		{
			VulkanZeroStruct(VkCommandBufferAllocateInfo, allocateInfo);
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.commandBufferCount = 1;
			allocateInfo.commandPool = m_CommandPool;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			
			vkAllocateCommandBuffers(m_renderDevice.m_Device, &allocateInfo, &commandBuffer);
		}
	}

	RenderCommandList::~RenderCommandList()
	{
		vkDestroyCommandPool(m_renderDevice.m_Device, m_CommandPool, nullptr);
		m_CommandPool = nullptr;
	}
}