#pragma once

#include "Platform/Window.h"
#include "RenderDevice.h"
#include "RenderBackend/RenderResource.h"
#include "RenderBackend/RenderDeviceChild.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <array>

namespace ZE::RenderBackend
{
	class RenderCommandList;

	class RenderWindow : public Platform::Window, public std::enable_shared_from_this<RenderWindow>, public RenderDeviceChild
	{
		friend class RenderDevice;
		
	public:

		RenderWindow(RenderDevice& renderDevice, const Settings& settings);
		virtual ~RenderWindow() override;

		bool Initialize();
		void Shutdown();

		virtual bool Resize(uint32_t width, uint32_t height) override;

		void BeginFrame();
		void EndFrame();

		void Present();

		const std::shared_ptr<Texture>& GetFrameSwapchainRenderTarget() const { return m_SwapchainTextures[m_SwapchainPresentImageIndex]; }
		const TextureDesc& GetSwapchainTextureDesc() const { return m_SwapchainBackbufferDesc; }

	private:

		bool CreateOrRecreateSwapchain();

	private:
		
		VkSwapchainKHR															m_Swapchain = nullptr;

		TextureDesc																m_SwapchainBackbufferDesc = {"swapchain render target"};

		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>					m_PresentCompleteSemaphores = {};
		std::array<VkSemaphore, RenderDevice::kSwapBufferCount>					m_RenderCompleteSemaphores = {};

		std::array<VkImage, RenderDevice::kSwapBufferCount>						m_Images = {};
		std::array<VkFence, RenderDevice::kSwapBufferCount>						m_Fences = {};

		uint32_t																m_SwapchainPresentImageIndex = 0u;

		std::array<std::shared_ptr<Texture>, RenderDevice::kSwapBufferCount>	m_SwapchainTextures = {};

		bool																	m_HadBeganRendering = false;
		bool																	m_HadResized = false;
	};
}
