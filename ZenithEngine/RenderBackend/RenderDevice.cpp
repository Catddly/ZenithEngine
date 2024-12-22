#include "RenderDevice.h"

#include "Render/Render.h"
#include "Engine/Engine.h"
#include "Platform/Window.h"
#include "RenderWindow.h"
#include "VulkanHelper.h"

#include "GLFW/glfw3.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include "vulkan/vulkan_win32.h"

#include <vector>
#include <string.h>
#include <ranges>
#include <algorithm>
#include <limits>

namespace ZE::RenderBackend
{
	constexpr static char const* const gEngineRequiredInstanceLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	constexpr static char const* const gEngineRequiredInstanceExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		//VK_KHR_SURFACE_EXTENSION_NAME,
		//VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	constexpr static char const* const gEngineRequiredDeviceLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	constexpr static char const* const gEngineRequiredDeviceExtensions[] = {
		// common
		//-------------------------------------------------------------------------
		//VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE2_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE_4_EXTENSION_NAME, // allow descriptor set has NOT consumed slot
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, // swapchain

		//VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,

		//VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, // buffer address

		// ray tracing
		//-------------------------------------------------------------------------
		//VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		//VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		//VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		//VK_KHR_RAY_QUERY_EXTENSION_NAME,
	};

	static VkBool32 VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		char const* messageTypeStr = "";
		switch (messageTypes)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: messageTypeStr = "[General]"; break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: messageTypeStr = "[Validation]"; break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: messageTypeStr = "[Performance]"; break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: messageTypeStr = "[DeviceAddessBinding]"; break;
		default: messageTypeStr = "[Unknown]"; break;
		}

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			ZE_LOG_INFO("Vulkan: {} {}", messageTypeStr, pCallbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			ZE_LOG_INFO("Vulkan:{} {}", messageTypeStr, pCallbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			ZE_LOG_WARNING("Vulkan:{} {}", messageTypeStr, pCallbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			ZE_LOG_ERROR("Vulkan: {} {}", messageTypeStr, pCallbackData->pMessage);
			break;

		default:
			ZE_CHECK(false);
			break;
		}

		return false;
	}

	static VkDebugUtilsMessengerCreateInfoEXT PopulateDebugMessageerCreateInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.pNext = nullptr;
		info.flags = VkFlags(0);
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		info.pfnUserCallback = VulkanDebugCallback;
		info.pUserData = nullptr;
		return info;
	}

	//-------------------------------------------------------------------------

	RenderDevice::RenderDevice(Render::RenderModule& renderModule, const Settings& settings)
		: m_RenderModule(renderModule), m_Settings(settings)
	{
	}

	bool RenderDevice::CreateInstance()
	{
		std::vector<const char*> requiredInstanceLayerArray;
		std::vector<const char*> requiredInstanceExtensionArray;

		for (uint32_t i = 0; i < sizeof(gEngineRequiredInstanceLayers) / sizeof(const char*); ++i)
		{
			requiredInstanceLayerArray.push_back(gEngineRequiredInstanceLayers[i]);
		}

		for (uint32_t i = 0; i < sizeof(gEngineRequiredInstanceExtensions) / sizeof(const char*); ++i)
		{
			requiredInstanceExtensionArray.push_back(gEngineRequiredInstanceExtensions[i]);
		}

		// TODO: verify glfw

		uint32_t glfwRequiredInstanceExtensionCount = 0;
		const char** pGLFWRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredInstanceExtensionCount);

		for (uint32_t i = 0; i < glfwRequiredInstanceExtensionCount; ++i)
		{
			auto iter = std::ranges::find_if(requiredInstanceExtensionArray, [pGLFWRequiredExtensions, i](const char* pExtension)
			{
				if (std::strcmp(pGLFWRequiredExtensions[i], pExtension) == 0)
				{
					return true;
				}
				return false;
			});

			if (iter == requiredInstanceExtensionArray.end())
			{
				requiredInstanceExtensionArray.push_back(pGLFWRequiredExtensions[i]);
			}
		}

		//-------------------------------------------------------------------------

		if (!CollectInstanceLayerProps(requiredInstanceLayerArray))
		{
			return false;
		}

		if (!CollectInstanceExtensionProps(requiredInstanceExtensionArray))
		{
			return false;
		}

		VulkanZeroStruct(VkApplicationInfo, appInfo);
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pEngineName = "Zenith Engine";
		appInfo.pApplicationName = "Unknown";
		appInfo.apiVersion = VK_API_VERSION_1_3;
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);

		VulkanZeroStruct(VkInstanceCreateInfo, instanceCI);
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.pNext = nullptr;
		instanceCI.flags = VkFlags(0);
		instanceCI.pApplicationInfo = &appInfo;

		auto debugMessagerCreateInfo = PopulateDebugMessageerCreateInfo();

		if (m_Settings.m_EnableValidationLayer)
		{
			instanceCI.pNext = (void*)&debugMessagerCreateInfo;
		}

		instanceCI.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayerArray.size());
		instanceCI.ppEnabledLayerNames = requiredInstanceLayerArray.data();
		instanceCI.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensionArray.size());
		instanceCI.ppEnabledExtensionNames = requiredInstanceExtensionArray.data();

		VulkanCheckSucceed(vkCreateInstance(&instanceCI, nullptr, &m_Inst));

		return true;
	}

	bool RenderDevice::CreateDebugLayer()
	{
		auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Inst, "vkCreateDebugUtilsMessengerEXT"));
		if (vkCreateDebugUtilsMessengerEXT)
		{
			auto debugMessengerCI = PopulateDebugMessageerCreateInfo();
			VulkanCheckSucceed(vkCreateDebugUtilsMessengerEXT(m_Inst, &debugMessengerCI, nullptr, &m_DebugUtilsMessenger));
		}

		return true;
	}

	bool RenderDevice::CreateWin32Surface()
	{
		// TODO: verify glfw

		auto pMainWindow = std::static_pointer_cast<Platform::Window>(m_RenderModule.GetMainRenderWindow());
		GLFWwindow* pGLFWWindow = reinterpret_cast<GLFWwindow*>(pMainWindow->GetNativeHandle());

		VulkanCheckSucceed(glfwCreateWindowSurface(m_Inst, pGLFWWindow, nullptr, &m_Surface));

		return true;
	}

	bool RenderDevice::PickPhysicalDevice()
	{
		// Enumerate available physical devices
		uint32_t pdCount = 0;
		VulkanCheckSucceed(vkEnumeratePhysicalDevices(m_Inst, &pdCount, nullptr));

		if (pdCount == 0)
		{
			ZE_LOG_ERROR("No suitable physical device found to render!");
			return false;
		}

		auto pdDevices = std::vector<VkPhysicalDevice>(pdCount);
		VulkanCheckSucceed(vkEnumeratePhysicalDevices(m_Inst, &pdCount, pdDevices.data()));

		for (uint32_t i = 0; i < pdCount; ++i)
		{
			auto const& pdDevice = pdDevices[i];

			VkPhysicalDeviceFeatures pdFeatures = {};
			vkGetPhysicalDeviceFeatures(pdDevice, &pdFeatures);

			VkPhysicalDeviceProperties pdProps = {};
			vkGetPhysicalDeviceProperties(pdDevice, &pdProps);

			VkPhysicalDeviceMemoryProperties pdMemoryProps = {};
			vkGetPhysicalDeviceMemoryProperties(pdDevice, &pdMemoryProps);

			uint32_t queueCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(pdDevice, &queueCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueProps(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(pdDevice, &queueCount, queueProps.data());

			auto& phyDevice = m_PhysicalDevices.emplace_back();
			phyDevice.m_pHandle = pdDevice;
			phyDevice.m_Features = pdFeatures;
			phyDevice.m_Props = pdProps;
			phyDevice.m_MemoryProps = pdMemoryProps;

			for (uint32_t qIndex = 0; qIndex < queueCount; ++qIndex)
			{
				phyDevice.m_QueueArray.emplace_back(qIndex, queueProps[qIndex]);
			}
		}

		// Calculate devices' pick score
		for (auto& pd : m_PhysicalDevices)
		{
			bool bHasValidPresentQueue = false;

			for (const auto& queue : pd.m_QueueArray)
			{
				VkBool32 supported = false;

				VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceSupportKHR(pd.m_pHandle, queue.m_Index, m_Surface, &supported));

				// is this physical device support present?
				if (queue.m_Props.queueCount > 0 &&
					queue.IsGraphicQueue() &&
					supported)
				{
					bHasValidPresentQueue = true;
					break;
				}
			}

			if (bHasValidPresentQueue)
			{
				if ((pd.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) != 0)
				{
					pd.m_PickScore = 10u;
				}
				else if ((pd.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) != 0)
				{
					pd.m_PickScore = 100u;
				}
				else if ((pd.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) != 0)
				{
					pd.m_PickScore = 1u;
				}
				else
				{
					ZE_LOG_ERROR("Found invalid physical device type: {}", static_cast<uint32_t>(pd.m_Props.deviceType));
					pd.m_PickScore = 0u;
				}
			}
			else
			{
				pd.m_PickScore = 0u;
			}
		}

		// Pick the most suitable physical device
		bool allPhysicalDeviceInvalid = true;

		uint32_t currPickScore = std::numeric_limits<uint32_t>::min();

		for (std::size_t i = 0; i < m_PhysicalDevices.size(); ++i)
		{
			auto const& pd = m_PhysicalDevices[i];

			if (pd.m_PickScore > 0)
			{
				allPhysicalDeviceInvalid = false;

				if (pd.m_PickScore > currPickScore)
				{
					m_PickedPhysicalDeviceIndex = (int32_t)i;
					currPickScore = pd.m_PickScore;
				}
			}
		}

		if (allPhysicalDeviceInvalid)
		{
			ZE_LOG_ERROR("No valid physical devices had been found. Render device can NOT be created.");
			return false;
		}

		ZE_LOG_INFO("Pick physical device info: \n\tname: {}\n\tdriver version: {}\n\tvendor id: {}",
			m_PhysicalDevices[m_PickedPhysicalDeviceIndex].m_Props.deviceName,
			m_PhysicalDevices[m_PickedPhysicalDeviceIndex].m_Props.driverVersion,
			m_PhysicalDevices[m_PickedPhysicalDeviceIndex].m_Props.vendorID
		);

		return true;
	}

	bool RenderDevice::CreateDevice()
	{
		// Collect device properties
		std::vector<const char*> requiredDeviceLayerArray;
		std::vector<const char*> requiredDeviceExtensionArray;

		for (uint32_t i = 0; i < sizeof(gEngineRequiredDeviceLayers) / sizeof(const char*); ++i)
		{
			requiredDeviceLayerArray.push_back(gEngineRequiredDeviceLayers[i]);
		}

		for (uint32_t i = 0; i < sizeof(gEngineRequiredDeviceExtensions) / sizeof(const char*); ++i)
		{
			requiredDeviceExtensionArray.push_back(gEngineRequiredDeviceExtensions[i]);
		}

		CollectDeviceLayerProps(requiredDeviceLayerArray);
		CollectDeviceExtensionProps(requiredDeviceExtensionArray);

		// Device queue creation info population
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs = {};
		std::vector<QueueFamily> deviceQueueFamilies = {};

		auto& physicalDevice = m_PhysicalDevices[m_PickedPhysicalDeviceIndex];

		float priorities[] = { 1.0f, 0.7f, 0.5f, 0.3f };

		for (auto const& qf : physicalDevice.m_QueueArray)
		{
			VkDeviceQueueCreateInfo dqCI = {};
			dqCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			dqCI.flags = VkFlags(0);
			dqCI.pNext = nullptr;

			if (qf.IsGraphicQueue())
			{
				dqCI.queueCount = std::min(4u, qf.m_Props.queueCount);
				dqCI.queueFamilyIndex = qf.m_Index;
				dqCI.pQueuePriorities = priorities;

				deviceQueueCIs.push_back(dqCI);
				deviceQueueFamilies.push_back(qf);
			}
			else if (qf.IsTransferQueue())
			{
				dqCI.queueCount = std::min(4u, qf.m_Props.queueCount);
				dqCI.queueFamilyIndex = qf.m_Index;
				dqCI.pQueuePriorities = priorities;

				deviceQueueCIs.push_back(dqCI);
				deviceQueueFamilies.push_back(qf);
			}
		}

		if (deviceQueueCIs.empty())
		{
			ZE_LOG_ERROR("Invalid physical device which not supports graphic queue!");
			return false;
		}

		// Physical device features2 validation
		//auto descriptor_indexing = VkPhysicalDeviceDescriptorIndexingFeaturesEXT{};
		//descriptor_indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

		//auto imageless_framebuffer = VkPhysicalDeviceImagelessFramebufferFeaturesKHR{};
		//imageless_framebuffer.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR;

		//auto buffer_address = VkPhysicalDeviceBufferDeviceAddressFeaturesEXT{};
		//buffer_address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;

		// TODO: pNext chain
		//VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
		//physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		//physicalDeviceFeatures2.pNext = &descriptor_indexing;
		//descriptor_indexing.pNext = &imageless_framebuffer;
		//imageless_framebuffer.pNext = &buffer_address;

		//vkGetPhysicalDeviceFeatures2(physicalDevice.m_pHandle, &physicalDeviceFeatures2);

		//ZE_CHECK(imageless_framebuffer.imagelessFramebuffer);
		//ZE_CHECK(descriptor_indexing.descriptorBindingPartiallyBound);
		//ZE_CHECK(buffer_address.bufferDeviceAddress);

		// Device creation
		VkDeviceCreateInfo deviceCI = {};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCI.flags = VkFlags(0);
		//deviceCI.pNext = &physicalDeviceFeatures2;

		deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCIs.size());
		deviceCI.pQueueCreateInfos = deviceQueueCIs.data();

		deviceCI.enabledLayerCount = static_cast<uint32_t>(requiredDeviceLayerArray.size());
		deviceCI.ppEnabledLayerNames = requiredDeviceLayerArray.data();
		deviceCI.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensionArray.size());
		deviceCI.ppEnabledExtensionNames = requiredDeviceExtensionArray.data();

		VulkanCheckSucceed(vkCreateDevice(physicalDevice.m_pHandle, &deviceCI, nullptr, &m_Device));

		// Fetch global device queue
		for (QueueFamily& deviceQueueFamily : deviceQueueFamilies)
		{
			if (deviceQueueFamily.IsGraphicQueue())
			{
				vkGetDeviceQueue(m_Device, deviceQueueFamily.m_Index, 0, &m_GraphicQueue);
				m_GraphicQueueFamilyIndex = deviceQueueFamily.m_Index;
				break;
			}
		}

		bool bHasNoTransferQueue = true;
		for (QueueFamily& deviceQueueFamily : deviceQueueFamilies)
		{
			if (deviceQueueFamily.IsTransferQueue())
			{
				vkGetDeviceQueue(m_Device, deviceQueueFamily.m_Index, 0, &m_TransferQueue);
				m_TransferQueueFamilyIndex = deviceQueueFamily.m_Index;
				bHasNoTransferQueue = false;
				break;
			}
		}

		if (bHasNoTransferQueue)
		{
			// No transfer queue, try fetch another graph queue
			for (QueueFamily& deviceQueueFamily : deviceQueueFamilies)
			{
				if (deviceQueueFamily.IsGraphicQueue())
				{
					vkGetDeviceQueue(m_Device, deviceQueueFamily.m_Index, 0, &m_TransferQueue);
					m_TransferQueueFamilyIndex = deviceQueueFamily.m_Index;
					break;
				}
			}
		}

		// TODO: Create global render command pools

		return true;
	}

	bool RenderDevice::CollectInstanceLayerProps(const std::vector<const char*>& requiredLayers)
	{
		uint32_t layerCount = 0;
		VulkanCheckSucceed(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		ZE_CHECK(layerCount > 0);

		std::vector<VkLayerProperties> layerProps(layerCount);
		VulkanCheckSucceed(vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data()));
		m_InstProps.m_LayerPropArray = layerProps;

		for (auto const& required : requiredLayers)
		{
			bool foundLayer = false;

			for (auto const& layer : layerProps)
			{
				if (strcmp(required, layer.layerName) == 0)
				{
					foundLayer = true;
					break;
				}
			}

			if (!foundLayer)
			{
				ZE_LOG_ERROR("Instance layer not found: {}", required);
				return false;
			}
		}

		return true;
	}

	bool RenderDevice::CollectInstanceExtensionProps(const std::vector<const char*>& requiredExtensions)
	{
		uint32_t extCount = 0;
		VulkanCheckSucceed(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr));

		ZE_CHECK(extCount > 0);

		std::vector<VkExtensionProperties> extProps(extCount);
		VulkanCheckSucceed(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProps.data()));
		m_InstProps.m_ExtensionPropArray = extProps;

		for (auto const& required : requiredExtensions)
		{
			bool foundExt = false;

			for (auto const& ext : extProps)
			{
				if (strcmp(required, ext.extensionName) == 0)
				{
					foundExt = true;
					break;
				}
			}

			if (!foundExt)
			{
				ZE_LOG_ERROR("Instance extension not found: {}", required);
				return false;
			}
		}

		return true;
	}

	bool RenderDevice::CollectDeviceLayerProps(const std::vector<const char*>& requiredLayers)
	{
		const auto& physicalDevice = m_PhysicalDevices[m_PickedPhysicalDeviceIndex];

		uint32_t layerCount = 0;
		VulkanCheckSucceed(vkEnumerateDeviceLayerProperties(physicalDevice.m_pHandle, &layerCount, nullptr));

		ZE_CHECK(layerCount > 0);

		std::vector<VkLayerProperties> layerProps(layerCount);
		VulkanCheckSucceed(vkEnumerateDeviceLayerProperties(physicalDevice.m_pHandle, &layerCount, layerProps.data()));
		m_DeviceProps.m_LayerPropArray = layerProps;

		for (auto const& required : requiredLayers)
		{
			bool foundLayer = false;

			for (auto const& layer : layerProps)
			{
				if (strcmp(required, layer.layerName) == 0)
				{
					foundLayer = true;
					break;
				}
			}

			if (!foundLayer)
			{
				ZE_LOG_ERROR("Device layer not found: {}", required);
				return false;
			}
		}

		return true;
	}

	bool RenderDevice::CollectDeviceExtensionProps(const std::vector<const char*>& requiredExtensions)
	{
		const auto& physicalDevice = m_PhysicalDevices[m_PickedPhysicalDeviceIndex];

		uint32_t extCount = 0;
		VulkanCheckSucceed(vkEnumerateDeviceExtensionProperties(physicalDevice.m_pHandle, nullptr, &extCount, nullptr));

		ZE_CHECK(extCount > 0);

		std::vector<VkExtensionProperties> extProps(extCount);
		VulkanCheckSucceed(vkEnumerateDeviceExtensionProperties(physicalDevice.m_pHandle, nullptr, &extCount, extProps.data()));
		m_DeviceProps.m_ExtensionPropArray = extProps;

		for (auto const& required : requiredExtensions)
		{
			bool foundExt = false;

			for (auto const& ext : extProps)
			{
				if (strcmp(required, ext.extensionName) == 0)
				{
					foundExt = true;
					break;
				}
			}

			if (!foundExt)
			{
				ZE_LOG_ERROR("Device extension not found: {}", required);
				return false;
			}
		}

		return true;
	}

	bool RenderDevice::Initialize()
	{
		if (!CreateInstance())
		{
			return false;
		}

		if (!CreateWin32Surface())
		{
			return false;
		}

		if (!CreateDebugLayer())
		{
			return false;
		}

		if (!PickPhysicalDevice())
		{
			return false;
		}

		if (!CreateDevice())
		{
			return false;
		}

		return true;
	}

	void RenderDevice::Shutdown()
	{
		vkDestroyDevice(m_Device, nullptr);

		if (m_DebugUtilsMessenger)
		{
			auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Inst, "vkDestroyDebugUtilsMessengerEXT"));
			ZE_CHECK(vkDestroyDebugUtilsMessengerEXT);
			vkDestroyDebugUtilsMessengerEXT(m_Inst, m_DebugUtilsMessenger, nullptr);
		}

		if (m_Surface)
		{
			vkDestroySurfaceKHR(m_Inst, m_Surface, nullptr);
		}

		vkDestroyInstance(m_Inst, nullptr);
	}

	//std::shared_ptr<RenderBackend::RenderWindow> RenderDevice::CreateSecondaryRenderWindow()
	//{
	//	auto pRenderWindow = std::shared_ptr<RenderBackend::RenderWindow>();
	//	SetRenderContent(pRenderWindow);
	//	return pRenderWindow;
	//}
}
