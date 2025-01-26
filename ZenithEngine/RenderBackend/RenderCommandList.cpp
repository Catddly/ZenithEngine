#include "RenderCommandList.h"

#include "RenderDevice.h"
#include "RenderResource.h"
#include "VulkanHelper.h"

namespace ZE::RenderBackend
{
	RenderCommandList::RenderCommandList(RenderDevice& renderDevice)
		: m_RenderDevice(renderDevice)
	{
		VulkanZeroStruct(VkCommandPoolCreateInfo, commandPoolCI);
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		// TODO: sepecify command list type
		commandPoolCI.queueFamilyIndex = m_RenderDevice.m_GraphicQueueFamilyIndex;

		VulkanCheckSucceed(vkCreateCommandPool(m_RenderDevice.m_Device, &commandPoolCI, nullptr, &m_CommandPool));
		
		VulkanZeroStruct(VkCommandBufferAllocateInfo, allocateInfo);
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(m_RenderDevice.m_Device, &allocateInfo, &m_CommandBuffer);
	}

	RenderCommandList::~RenderCommandList()
	{
		vkDestroyCommandPool(m_RenderDevice.m_Device, m_CommandPool, nullptr);
		m_CommandPool = nullptr;
	}

	bool RenderCommandList::BeginRecord()
	{
		ZE_CHECK(!m_bIsCommandRecording);
		VulkanZeroStruct(VkCommandBufferBeginInfo, beginInfo);
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VulkanCheckSucceed(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

		m_bIsCommandRecording = true;
		return true;
	}

	void RenderCommandList::EndRecord()
	{
		ZE_CHECK(m_bIsCommandRecording);
		VulkanCheckSucceed(vkEndCommandBuffer(m_CommandBuffer));
		m_bIsCommandRecording = false;
	}

	// Copy Commands

	void RenderCommandList::CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t srcOffset)
	{
		ZE_CHECK(m_bIsCommandRecording);

		VkBufferCopy bufferCopy;
		bufferCopy.size = pDstBuffer->m_Desc.m_Size;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.dstOffset = 0;

		vkCmdCopyBuffer(m_CommandBuffer, pSrcBuffer->m_Handle, pDstBuffer->m_Handle, 1, &bufferCopy);
	}
}