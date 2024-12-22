#pragma once

#include "RenderDevice.h"

#include "vulkan/vulkan_core.h"

#include <array>

namespace ZE::RenderBackend
{
	class RenderDevice;

	class RenderCommandList
	{
	public:

		RenderCommandList(RenderDevice& renderDevice);
		virtual ~RenderCommandList();

	private:

		RenderDevice&													m_renderDevice;

		VkCommandPool													m_CommandPool = nullptr;
		std::array<VkCommandBuffer, RenderDevice::kSwapBufferCount>		m_CommandBufferArray = {};
	};
}
