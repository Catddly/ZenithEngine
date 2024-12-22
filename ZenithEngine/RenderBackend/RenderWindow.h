#pragma once

#include "Platform/Window.h"
#include "RenderDevice.h"

#include "vulkan/vulkan_core.h"

#include <memory>
#include <array>
#include <limits>

namespace ZE::RenderBackend
{
	class RenderWindow : public Platform::Window, public std::enable_shared_from_this<RenderWindow>
	{
		static constexpr uint64_t kInfiniteWaitTime = std::numeric_limits<uint64_t>::max();

	public:

		RenderWindow(RenderDevice& renderDevice, const Settings& settings);
		virtual ~RenderWindow();

		bool Initialize();
		void Shutdown();

		void BeginRender();
		void Render(RenderCommandList& commandList);
		void EndRender();

		void Present();

	private:

		bool CreateOrRecreateSwapchain();

	private:

		RenderDevice&												m_renderDevice;

		VkSwapchainKHR												m_Swapchain = nullptr;

		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>		m_PresentCompleteSemaphoreArray = {};
		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>		m_RenderCompleteSemaphoreArray = {};

		std::array<VkImage, RenderDevice::kSwapBufferCount>			m_ImageArray = {};

		std::array<VkFence, RenderDevice::kSwapBufferCount>			m_FenceArray = {};

		uint64_t													m_FrameCounter = 0;
	};
}
