#include "RenderWindow.h"

#include "Core/Assertion.h"
#include "RenderDevice.h"
#include "VulkanHelper.h"

#include "glm/ext/vector_int2.hpp"

namespace ZE::RenderBackend
{
	RenderWindow::RenderWindow(RenderDevice& renderDevice, const Settings& settings)
		: Window(settings)
	{
		SetRenderDevice(&renderDevice);
	}

	RenderWindow::~RenderWindow()
	{
		ZE_ASSERT(!m_Swapchain);
	}

	bool RenderWindow::Initialize()
	{
		if (!CreateOrRecreateSwapchain())
		{
			return false;
		}

		return true;
	}

	void RenderWindow::Shutdown()
	{
		for (auto& fence : m_Fences)
		{
			if (!fence)
			{
				continue;
			}
			
			vkDestroyFence(GetRenderDevice().GetNativeDevice(), fence, nullptr);
		}

		for (auto& swapchainTex : m_SwapchainTextures)
		{
			swapchainTex->m_Handle = nullptr;
		}

		for (auto& image : m_Images)
		{
			image = nullptr;
		}

		for (auto& semaphore : m_RenderCompleteSemaphores)
		{
			if (!semaphore)
			{
				continue;
			}
			
			vkDestroySemaphore(GetRenderDevice().GetNativeDevice(), semaphore, nullptr);
			semaphore = nullptr;
		}

		for (auto& semaphore : m_PresentCompleteSemaphores)
		{
			if (!semaphore)
			{
				continue;
			}
			
			vkDestroySemaphore(GetRenderDevice().GetNativeDevice(), semaphore, nullptr);
			semaphore = nullptr;
		}

		if (GetRenderDevice().GetNativeDevice() && m_Swapchain)
		{
			vkDestroySwapchainKHR(GetRenderDevice().GetNativeDevice(), m_Swapchain, nullptr);
		}

		m_Swapchain = nullptr;
	}
	
	bool RenderWindow::Resize(uint32_t width, uint32_t height)
	{
		m_HadResized = Window::Resize(width, height);
		return m_HadResized;
	}

	void RenderWindow::BeginFrame()
	{
		ZE_ASSERT(!m_HadBeganRendering);

		if (m_HadResized)
		{
			CreateOrRecreateSwapchain();
			m_HadResized = false;
		}

		const uint32_t frameIndex = GetRenderDevice().GetFrameIndex(); 
		
		// Wait for until command buffer had finished execution
		VulkanCheckSucceed(vkWaitForFences(GetRenderDevice().GetNativeDevice(), 1, &m_Fences[frameIndex], VK_TRUE, RenderDevice::kInfiniteWaitTime));
		VulkanCheckSucceed(vkResetFences(GetRenderDevice().GetNativeDevice(), 1, &m_Fences[frameIndex]));
		
		//-------------------------------------------------------------------------
		
		VkResult acquireImageResult = vkAcquireNextImageKHR(GetRenderDevice().GetNativeDevice(), m_Swapchain, RenderDevice::kInfiniteWaitTime, m_PresentCompleteSemaphores[frameIndex], nullptr, &m_SwapchainPresentImageIndex);

		if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			CreateOrRecreateSwapchain();
		}
		else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR)
		{
			ZE_LOG_FATAL("Failed to acquire next frame image.");
		}

		m_HadBeganRendering = true;
	}
	
	void RenderWindow::EndFrame()
	{
		ZE_ASSERT(m_HadBeganRendering);
		m_HadBeganRendering = false;
	}

	void RenderWindow::Present()
	{
		const uint32_t frameIndex = GetRenderDevice().GetFrameIndex(); 
		
		VulkanZeroStruct(VkPresentInfoKHR, presentInfo);
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderCompleteSemaphores[frameIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_SwapchainPresentImageIndex;

		VkResult result = vkQueuePresentKHR(GetRenderDevice().m_GraphicQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			CreateOrRecreateSwapchain();
		}
		else if (result != VK_SUCCESS)
		{
			ZE_ASSERT_LOG(false, "Failed to present swapchain image!");
		}
	}

	bool RenderWindow::CreateOrRecreateSwapchain()
	{
		GetRenderDevice().WaitUntilIdle();
		
		const auto& physicalDevice = GetRenderDevice().GetPhysicalDevice().m_Handle;
		VkSwapchainKHR oldSwapchain = m_Swapchain;

		uint32_t surfaceFormatCount = 0;
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, GetRenderDevice().m_Surface, &surfaceFormatCount, nullptr));

		if (surfaceFormatCount == 0)
		{
			ZE_LOG_ERROR("Surface support zero valid format!");
			return false;
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, GetRenderDevice().m_Surface, &surfaceFormatCount, surfaceFormats.data()));

		VkSurfaceFormatKHR pickFormat = {};
		VkFormat desireFormat = VK_FORMAT_B8G8R8A8_UNORM;
		VkFormat pickedFormat = VK_FORMAT_UNDEFINED;

		if (surfaceFormatCount == 1)
		{
			if (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				pickFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
				pickFormat.colorSpace = surfaceFormats[0].colorSpace;
				pickedFormat = VK_FORMAT_B8G8R8A8_UNORM;
			}
		}
		else
		{
			for (auto const& format : surfaceFormats)
			{
				if (format.format == desireFormat)
				{
					pickFormat.format = desireFormat;
					pickedFormat = pickFormat.format;
					break;
				}
			}
		}

		// get image count and extent
		//-------------------------------------------------------------------------

		VkSurfaceCapabilitiesKHR surfaceCaps = {};
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, GetRenderDevice().m_Surface, &surfaceCaps));

		constexpr uint32_t desiredSwapchainBufferCount = 2;

		uint32_t const imageCount = std::max(desiredSwapchainBufferCount, surfaceCaps.minImageCount);
		if (imageCount > surfaceCaps.maxImageCount)
		{
			ZE_LOG_ERROR("Vulkan swapchain image count exceed max image count limit: %u", surfaceCaps.maxImageCount);
			return false;
		}

		glm::ivec2 extent{0};

		if (surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			extent.x = static_cast<int>(surfaceCaps.currentExtent.width);
		}
		if (surfaceCaps.currentExtent.height != std::numeric_limits<uint32_t>::max())
		{
			extent.y = static_cast<int>(surfaceCaps.currentExtent.height);
		}

		ZE_ASSERT((extent != glm::ivec2{0, 0}));

		// get present mode
		//-------------------------------------------------------------------------

		uint32_t supportedPresentModeCount = 0;
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, GetRenderDevice().m_Surface, &supportedPresentModeCount, nullptr));

		if (supportedPresentModeCount == 0)
		{
			ZE_LOG_ERROR("No present mode support for this swapchain!");
			return false;
		}

		std::vector<VkPresentModeKHR> supportedPresentModes(supportedPresentModeCount);
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, GetRenderDevice().m_Surface, &supportedPresentModeCount, supportedPresentModes.data()));

		// TODO: configurable
		constexpr bool bEnableVSync = false;

		// choose present modes by vsync, the one at the front will be chosen first if they both supported by the surface.
		// more info: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
		std::vector<VkPresentModeKHR> presentModes;
		if (bEnableVSync)
		{
			presentModes = { VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR };
		}
		else
		{
			presentModes = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR };
		}

		VkPresentModeKHR pickPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (auto const& pm : presentModes)
		{
			if (auto iter = std::ranges::find(supportedPresentModes, pm); iter != supportedPresentModes.end())
			{
				pickPresentMode = pm;
			}
		}

		// get surface transform
		//-------------------------------------------------------------------------

		VkSurfaceTransformFlagBitsKHR transformFlag = {};
		if ((surfaceCaps.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
		{
			transformFlag = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			transformFlag = surfaceCaps.currentTransform;
		}

		// create swapchain
		//-------------------------------------------------------------------------

		VulkanZeroStruct(VkSwapchainCreateInfoKHR, swapchainCI);
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.flags = 0;
		swapchainCI.pNext = nullptr;
		swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCI.presentMode = pickPresentMode;
		swapchainCI.clipped = true;
		swapchainCI.preTransform = transformFlag;
		swapchainCI.surface = GetRenderDevice().m_Surface;
		swapchainCI.minImageCount = imageCount;

		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.imageExtent = { static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y) };
		swapchainCI.imageFormat = pickFormat.format;
		swapchainCI.imageColorSpace = pickFormat.colorSpace;
		swapchainCI.imageArrayLayers = 1;

		swapchainCI.oldSwapchain = oldSwapchain;

		VulkanCheckSucceed(vkCreateSwapchainKHR(GetRenderDevice().GetNativeDevice(), &swapchainCI, nullptr, &m_Swapchain));

		m_SwapchainBackbufferDesc.m_Size = { swapchainCI.imageExtent.width, swapchainCI.imageExtent.height };
		m_SwapchainBackbufferDesc.m_Format = pickFormat.format;
		m_SwapchainBackbufferDesc.m_Usage = 1 << static_cast<uint8_t>(ETextureUsage::Color);

		// delete old swapchain
		//-------------------------------------------------------------------------

		if (oldSwapchain)
		{
			for (auto& image : m_Images)
			{
				image = nullptr;
			}

			for (auto& semaphore : m_RenderCompleteSemaphores)
			{
				vkDestroySemaphore(GetRenderDevice().GetNativeDevice(), semaphore, nullptr);
				semaphore = nullptr;
			}

			for (auto& semaphore : m_PresentCompleteSemaphores)
			{
				vkDestroySemaphore(GetRenderDevice().GetNativeDevice(), semaphore, nullptr);
				semaphore = nullptr;
			}

			for (auto& fence : m_Fences)
			{
				vkDestroyFence(GetRenderDevice().GetNativeDevice(), fence, nullptr);
			}

			for (auto& swapchainTex : m_SwapchainTextures)
			{
				swapchainTex->m_Handle = nullptr;
			}

			vkDestroySwapchainKHR(GetRenderDevice().GetNativeDevice(), oldSwapchain, nullptr);
		}

		// fetch swapchain images
		//-------------------------------------------------------------------------

		uint32_t swapchainImageCount = 0;
		VulkanCheckSucceed(vkGetSwapchainImagesKHR(GetRenderDevice().GetNativeDevice(), m_Swapchain, &swapchainImageCount, nullptr));

		if (swapchainImageCount == 0)
		{
			vkDestroySwapchainKHR(GetRenderDevice().GetNativeDevice(), m_Swapchain, nullptr);
			return false;
		}

		ZE_ASSERT(m_Images.size() == static_cast<std::size_t>(swapchainImageCount));
		VulkanCheckSucceed(vkGetSwapchainImagesKHR(GetRenderDevice().GetNativeDevice(), m_Swapchain, &swapchainImageCount, m_Images.data()));

		for (uint32_t i = 0; i < m_SwapchainTextures.size(); ++i)
		{
			auto& pCurrTex = m_SwapchainTextures[i];
			if (pCurrTex)
			{
				pCurrTex.reset();
			}

			pCurrTex = std::shared_ptr<Texture>(new Texture(GetRenderDevice(), m_SwapchainBackbufferDesc));
			pCurrTex->m_Handle = m_Images[i];
		}

		// create new semaphores
		//-------------------------------------------------------------------------

		VulkanZeroStruct(VkSemaphoreCreateInfo, semaphoreCI);
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < m_RenderCompleteSemaphores.size(); ++i)
		{
			VulkanCheckSucceed(vkCreateSemaphore(GetRenderDevice().GetNativeDevice(), &semaphoreCI, nullptr, &m_RenderCompleteSemaphores[i]));
			VulkanCheckSucceed(vkCreateSemaphore(GetRenderDevice().GetNativeDevice(), &semaphoreCI, nullptr, &m_PresentCompleteSemaphores[i]));
		}

		// create new fences
		//-------------------------------------------------------------------------

		VulkanZeroStruct(VkFenceCreateInfo, fenceCI);
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (auto& fence : m_Fences)
		{
			vkCreateFence(GetRenderDevice().GetNativeDevice(), &fenceCI, nullptr, &fence);
		}

		return true;
	}
}
