#pragma once

#include "Platform/Window.h"
#include "RenderDevice.h"
#include "RenderBackend/RenderResource.h"

#include "vulkan/vulkan_core.h"

#include <memory>
#include <array>
#include <limits>

namespace ZE::RenderBackend
{
	class RenderCommandList;

	class RenderWindow : public Platform::Window, public std::enable_shared_from_this<RenderWindow>
	{
	public:

		RenderWindow(RenderDevice& renderDevice, const Settings& settings);
		virtual ~RenderWindow();

		bool Initialize();
		void Shutdown();

		void BeginRender();
		void Render(RenderCommandList& commandList);
		void EndRender();

		void Present();

		inline Texture* GetSwapchainRenderTarget() const { return m_SwapchainTextures[m_FrameCounter % RenderDevice::kSwapBufferCount]; }

	private:

		bool CreateOrRecreateSwapchain();

	private:

		RenderDevice&												m_RenderDevice;

		VkSwapchainKHR												m_Swapchain = nullptr;

		TextureDesc													m_SwapchainBackbufferDesc;

		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>		m_PresentCompleteSemaphoreArray = {};
		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>		m_RenderCompleteSemaphoreArray = {};

		std::array<VkImage, RenderDevice::kSwapBufferCount>			m_ImageArray = {};

		std::array<VkFence, RenderDevice::kSwapBufferCount>			m_FenceArray = {};

		uint64_t													m_FrameCounter = 0;

		std::array<Texture*, RenderDevice::kSwapBufferCount>		m_SwapchainTextures = {};
	};
}
