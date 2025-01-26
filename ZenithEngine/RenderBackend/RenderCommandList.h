#pragma once

#include "vulkan/vulkan_core.h"

#include <array>

namespace ZE::RenderBackend
{
	class RenderDevice;

	class Buffer;

	class RenderCommandList
	{
		friend class RenderDevice;

	public:

		RenderCommandList(RenderDevice& renderDevice);
		virtual ~RenderCommandList();

		bool BeginRecord();
		void EndRecord();

		// transfer commands
		void CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t srcOffset = 0);

	private:

		RenderDevice&					m_RenderDevice;

		VkCommandPool					m_CommandPool = nullptr;
		VkCommandBuffer					m_CommandBuffer = nullptr;
		bool							m_bIsCommandRecording = false;
	};
}
