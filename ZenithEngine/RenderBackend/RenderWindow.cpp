#include "RenderWindow.h"

#include "Core/Assertion.h"
#include "RenderDevice.h"
#include "VulkanHelper.h"

#include "glm/ext/vector_int2.hpp"

namespace ZE::RenderBackend
{
	RenderWindow::RenderWindow(RenderDevice& renderDevice, const Settings& settings)
		: m_RenderDevice(renderDevice), Window(settings)
	{
	}

	RenderWindow::~RenderWindow()
	{
		ZE_CHECK(!m_Swapchain);
		ZE_CHECK(m_ImageArray.empty());
		ZE_CHECK(m_PresentCompleteSemaphoreArray.empty());
		ZE_CHECK(m_RenderCompleteSemaphoreArray.empty());
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
		vkDeviceWaitIdle(m_RenderDevice.m_Device);

		for (auto& fence : m_FenceArray)
		{
			vkDestroyFence(m_RenderDevice.m_Device, fence, nullptr);
		}

		for (auto& image : m_ImageArray)
		{
			image = nullptr;
		}

		for (auto& semaphore : m_RenderCompleteSemaphoreArray)
		{
			vkDestroySemaphore(m_RenderDevice.m_Device, semaphore, nullptr);
			semaphore = nullptr;
		}

		for (auto& semaphore : m_PresentCompleteSemaphoreArray)
		{
			vkDestroySemaphore(m_RenderDevice.m_Device, semaphore, nullptr);
			semaphore = nullptr;
		}

		vkDestroySwapchainKHR(m_RenderDevice.m_Device, m_Swapchain, nullptr);

		m_Swapchain = nullptr;
	}

	void RenderWindow::BeginRender()
	{
		const uint64_t frameIndex = m_FrameCounter % RenderDevice::kSwapBufferCount;

		// Wait for until command buffer had finished execution
		VulkanCheckSucceed(vkWaitForFences(m_RenderDevice.m_Device, 1, &m_FenceArray[frameIndex], VK_TRUE, RenderDevice::kInfiniteWaitTime));
	
		//-------------------------------------------------------------------------
		
		uint32_t imageIndex;
		VkResult acquireImageResult = vkAcquireNextImageKHR(m_RenderDevice.m_Device, m_Swapchain, RenderDevice::kInfiniteWaitTime, m_PresentCompleteSemaphoreArray[frameIndex], m_FenceArray[frameIndex], &imageIndex);

		if (acquireImageResult != VK_ERROR_OUT_OF_DATE_KHR)
		{
			vkDeviceWaitIdle(m_RenderDevice.m_Device);
			CreateOrRecreateSwapchain();
		}
		else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR)
		{
			ZE_LOG_FATAL("Failed to acquire next frame image.");
		}
	}

	void RenderWindow::Render(RenderCommandList& commandList)
	{


	}

	void RenderWindow::EndRender()
	{
		//m_FrameCounter += 1;
	}

	void RenderWindow::Present()
	{
		m_FrameCounter += 1;
	}

	bool RenderWindow::CreateOrRecreateSwapchain()
	{
		const auto& physicalDevice = m_RenderDevice.GetPhysicalDevice().m_Handle;
		VkSwapchainKHR oldSwapchain = m_Swapchain;

		vkDeviceWaitIdle(m_RenderDevice.m_Device);

		uint32_t surfaceFormatCount = 0;
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_RenderDevice.m_Surface, &surfaceFormatCount, nullptr));

		if (surfaceFormatCount == 0)
		{
			ZE_LOG_ERROR("Surface support zero valid format!");
			return false;
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_RenderDevice.m_Surface, &surfaceFormatCount, surfaceFormats.data()));

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
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_RenderDevice.m_Surface, &surfaceCaps));

		constexpr uint32_t desiredSwapchainBufferCount = 2;

		uint32_t const imageCount = std::max(desiredSwapchainBufferCount, surfaceCaps.minImageCount);
		if (imageCount > surfaceCaps.maxImageCount)
		{
			ZE_LOG_ERROR("Vulkan swapchain image count exceed max image count limit: %u", surfaceCaps.maxImageCount);
			return false;
		}

		glm::ivec2 extent{ 0 };

		if (surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			extent.x = surfaceCaps.currentExtent.width;
		}
		if (surfaceCaps.currentExtent.height != std::numeric_limits<uint32_t>::max())
		{
			extent.y = surfaceCaps.currentExtent.height;
		}

		ZE_CHECK((extent != glm::ivec2{ 0, 0 }));

		// get present mode
		//-------------------------------------------------------------------------

		uint32_t supportedPresentModeCount = 0;
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_RenderDevice.m_Surface, &supportedPresentModeCount, nullptr));

		if (supportedPresentModeCount == 0)
		{
			ZE_LOG_ERROR("No present mode support for this swapchain!");
			return false;
		}

		std::vector<VkPresentModeKHR> supportedPresentModes(supportedPresentModeCount);
		VulkanCheckSucceed(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_RenderDevice.m_Surface, &supportedPresentModeCount, supportedPresentModes.data()));

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
		swapchainCI.flags = VkFlags(0);
		swapchainCI.pNext = nullptr;
		swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCI.presentMode = pickPresentMode;
		swapchainCI.clipped = true;
		swapchainCI.preTransform = transformFlag;
		swapchainCI.surface = m_RenderDevice.m_Surface;
		swapchainCI.minImageCount = imageCount;

		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.imageExtent = { (uint32_t)extent.x, (uint32_t)extent.y };
		swapchainCI.imageFormat = pickFormat.format;
		swapchainCI.imageColorSpace = pickFormat.colorSpace;
		swapchainCI.imageArrayLayers = 1;

		swapchainCI.oldSwapchain = oldSwapchain;

		VulkanCheckSucceed(vkCreateSwapchainKHR(m_RenderDevice.m_Device, &swapchainCI, nullptr, &m_Swapchain));

		m_SwapchainBackbufferDesc.m_Size = { swapchainCI.imageExtent.width, swapchainCI.imageExtent.height };
		m_SwapchainBackbufferDesc.m_Format = pickFormat.format;

		// delete old swapchain
		//-------------------------------------------------------------------------

		if (oldSwapchain)
		{
			for (auto& image : m_ImageArray)
			{
				image = nullptr;
			}

			for (auto& semaphore : m_RenderCompleteSemaphoreArray)
			{
				vkDestroySemaphore(m_RenderDevice.m_Device, semaphore, nullptr);
				semaphore = nullptr;
			}

			for (auto& semaphore : m_PresentCompleteSemaphoreArray)
			{
				vkDestroySemaphore(m_RenderDevice.m_Device, semaphore, nullptr);
				semaphore = nullptr;
			}

			for (auto& fence : m_FenceArray)
			{
				vkDestroyFence(m_RenderDevice.m_Device, fence, nullptr);
			}

			vkDestroySwapchainKHR(m_RenderDevice.m_Device, oldSwapchain, nullptr);
		}

		// fetch swapchain images
		//-------------------------------------------------------------------------

		uint32_t swapchainImageCount = 0;
		VulkanCheckSucceed(vkGetSwapchainImagesKHR(m_RenderDevice.m_Device, m_Swapchain, &swapchainImageCount, nullptr));

		if (swapchainImageCount == 0)
		{
			vkDestroySwapchainKHR(m_RenderDevice.m_Device, m_Swapchain, nullptr);
			return false;
		}

		ZE_CHECK(m_ImageArray.size() == (std::size_t)swapchainImageCount);
		VulkanCheckSucceed(vkGetSwapchainImagesKHR(m_RenderDevice.m_Device, m_Swapchain, &swapchainImageCount, m_ImageArray.data()));

		for (uint32_t i = 0; i < m_SwapchainTextures.size(); ++i)
		{
			auto& pCurrTex = m_SwapchainTextures[i];
			if (pCurrTex)
			{
				delete pCurrTex;
			}

			pCurrTex = new Texture(m_RenderDevice, m_SwapchainBackbufferDesc);
			pCurrTex->m_Handle = m_ImageArray[i];
		}

		// create new semaphores
		//-------------------------------------------------------------------------

		VulkanZeroStruct(VkSemaphoreCreateInfo, semaphoreCI);
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < m_RenderCompleteSemaphoreArray.size(); ++i)
		{
			VulkanCheckSucceed(vkCreateSemaphore(m_RenderDevice.m_Device, &semaphoreCI, nullptr, &m_RenderCompleteSemaphoreArray[i]));
			VulkanCheckSucceed(vkCreateSemaphore(m_RenderDevice.m_Device, &semaphoreCI, nullptr, &m_PresentCompleteSemaphoreArray[i]));
		}

		// create new fences
		//-------------------------------------------------------------------------

		VulkanZeroStruct(VkFenceCreateInfo, fenceCI);
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (auto& fence : m_FenceArray)
		{
			vkCreateFence(m_RenderDevice.m_Device, &fenceCI, nullptr, &fence);
		}

		return true;
	}
}
