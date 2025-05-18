#include "RenderDevice.h"

#include "Render/Render.h"
#include "Platform/Window.h"
#include "RenderWindow.h"
#include "VulkanHelper.h"
#include "RenderCommandList.h"
#include "DescriptorCache.h"

#include <GLFW/glfw3.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
// Uncomment to debug memory leaking
// #define VMA_DEBUG_LOG(format, ...) do { char buffer[256]; std::sprintf(buffer, format, __VA_ARGS__); ZE_LOG_INFO("{}", buffer); } while(false);
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#include <vector>
#include <array>
#include <ranges>
#include <iterator>
#include <algorithm>
#include <limits>
#include <string_view>

// Expose operator""sv()
using namespace std::string_view_literals;

namespace 
{
	// TODO: move it to core module
	bool IsGLFWInitialized()
	{
		int major = 0, minor = 0, rev = 0;
		glfwGetVersion(&major, &minor, &rev);
		return major != 0 || minor != 0 || rev != 0;
	}
}

namespace ZE::RenderBackend
{
	constexpr static uint32_t gVulkanVersion = VK_MAKE_VERSION(1, 3, 0);

	constexpr static std::array gEngineRequiredInstanceLayers = {
		"VK_LAYER_KHRONOS_validation"sv,
	};

	constexpr static std::array gEngineRequiredInstanceExtensions = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		//VK_KHR_SURFACE_EXTENSION_NAME,
		//VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	constexpr static std::array gEngineRequiredDeviceLayers = {
		"VK_LAYER_KHRONOS_validation"sv,
	};

	constexpr static std::array gEngineRequiredDeviceExtensions = {
		// common
		//-------------------------------------------------------------------------
		//VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE2_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE_4_EXTENSION_NAME, // allow descriptor set has NOT consumed slot
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, // swapchain
		
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, // allow postponing creation of renderpass

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
			ZE_ASSERT(false);
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
		
		std::ranges::for_each(gEngineRequiredInstanceLayers, [&](auto layer)
		{
			requiredInstanceLayerArray.push_back(layer.data());
			ZE_LOG_INFO("Required instance layer: {}", layer);
		});

		std::ranges::for_each(gEngineRequiredInstanceExtensions, [&](auto extension)
		{
			requiredInstanceExtensionArray.push_back(extension);
			ZE_LOG_INFO("Required instance extension: {}", extension); 
		});

		ZE_EXEC_ASSERT(IsGLFWInitialized());
		
		uint32_t glfwRequiredInstanceExtensionCount = 0;
		const char** pGLFWRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredInstanceExtensionCount);
		if (!pGLFWRequiredExtensions)
		{
			ZE_LOG_ERROR("Failed to get GLFW required instance extensions");
			return false;
		}

		for (uint32_t i = 0; i < glfwRequiredInstanceExtensionCount; ++i)
		{
			ZE_LOG_INFO("GLFW required instance extension: {}", pGLFWRequiredExtensions[i]);
			auto iter = std::ranges::find_if(requiredInstanceExtensionArray, [pGLFWRequiredExtensions, i](auto extension)
			{
				std::string_view glfwExtension = pGLFWRequiredExtensions[i];
				if (glfwExtension == extension)
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
			ZE_LOG_ERROR("Failed to collect instance layer properties");
			return false;
		}

		if (!CollectInstanceExtensionProps(requiredInstanceExtensionArray))
		{
			ZE_LOG_ERROR("Failed to collect instance extension properties");
			return false;
		}

		VulkanZeroStruct(VkApplicationInfo, appInfo);
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pEngineName = "Zenith Engine";
		appInfo.pApplicationName = "Unknown";
		appInfo.apiVersion = gVulkanVersion;
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

		ZE_LOG_INFO("Vulkan instance created successfully");
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

		auto pMainWindow = std::static_pointer_cast<Platform::Window>(m_RenderModule.get().GetMainRenderWindow());
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
			phyDevice.m_Handle = pdDevice;
			phyDevice.m_Features = pdFeatures;
			phyDevice.m_Props = pdProps;
			phyDevice.m_MemoryProps = pdMemoryProps;

			for (uint32_t qIndex = 0; qIndex < queueCount; ++qIndex)
			{
				phyDevice.m_QueueArray.emplace_back(qIndex, queueProps[qIndex]);
			}
		}

		std::ranges::for_each(m_PhysicalDevices | std::views::filter([&](const auto& phyDevice)
		{
			return std::ranges::any_of(phyDevice.m_QueueArray, [&](const auto& queue)
			{
				VkBool32 supported = false;
				VulkanCheckSucceed(vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice.m_Handle, queue.m_Index, m_Surface, &supported));

				return queue.m_Props.queueCount > 0 && queue.IsGraphicQueue() && supported;
			});
		}), [](auto& phyDevice)
		{
			if ((phyDevice.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) != 0)
			{
				phyDevice.m_PickScore = 10u;
			}
			else if ((phyDevice.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) != 0)
			{
				phyDevice.m_PickScore = 100u;
			}
			else if ((phyDevice.m_Props.deviceType & VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) != 0)
			{
				phyDevice.m_PickScore = 1u;
			}
			else
			{
				ZE_LOG_ERROR("Found invalid physical device type: {}", static_cast<uint32_t>(phyDevice.m_Props.deviceType));
				phyDevice.m_PickScore = 0u;
			}
		});
		
		// Pick the most suitable physical device
		bool bAllPhysicalDeviceInvalid = true;
		
		auto validDevices = m_PhysicalDevices | std::views::filter([](const auto& phyDevice)
		{
			return phyDevice.m_PickScore > 0u;
		});
		if (!validDevices.empty())
		{
			auto bestDevice = std::ranges::max_element(validDevices, {}, &PhysicalDevice::m_PickScore);
			m_PickedPhysicalDeviceIndex = static_cast<int>(bestDevice.base() - m_PhysicalDevices.begin());
			bAllPhysicalDeviceInvalid = false;
		}

		if (bAllPhysicalDeviceInvalid)
		{
			ZE_LOG_ERROR("No valid physical devices had been found. Render device can NOT be created.");
			return false;
		}

		ZE_LOG_INFO("Picked physical device: \n\tname: {}\n\tdriver version: {}\n\tvendor id: {}",
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

		for (auto layer : gEngineRequiredDeviceLayers)
		{
			requiredDeviceLayerArray.push_back(layer.data());
		}

		for (auto extension : gEngineRequiredDeviceExtensions)
		{
			requiredDeviceExtensionArray.push_back(extension);
		}

		if (!CollectDeviceLayerProps(requiredDeviceLayerArray))
		{
			return false;
		}
		
		if (!CollectDeviceExtensionProps(requiredDeviceExtensionArray))
		{
			return false;
		}

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

		auto dynamicRenderingFeature = VkPhysicalDeviceDynamicRenderingFeaturesKHR{};
		dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		physicalDeviceFeatures2.pNext = &dynamicRenderingFeature;
		// descriptor_indexing.pNext = &imageless_framebuffer;
		// imageless_framebuffer.pNext = &buffer_address;

		vkGetPhysicalDeviceFeatures2(GetPhysicalDevice().m_Handle, &physicalDeviceFeatures2);

		ZE_ASSERT(dynamicRenderingFeature.dynamicRendering);
		//ZE_ASSERT(descriptor_indexing.descriptorBindingPartiallyBound);
		//ZE_ASSERT(buffer_address.bufferDeviceAddress);

		// Device creation
		VkDeviceCreateInfo deviceCI = {};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCI.flags = VkFlags(0);
		deviceCI.pNext = &physicalDeviceFeatures2;

		deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCIs.size());
		deviceCI.pQueueCreateInfos = deviceQueueCIs.data();

		deviceCI.enabledLayerCount = static_cast<uint32_t>(requiredDeviceLayerArray.size());
		deviceCI.ppEnabledLayerNames = requiredDeviceLayerArray.data();
		deviceCI.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensionArray.size());
		deviceCI.ppEnabledExtensionNames = requiredDeviceExtensionArray.data();

		VulkanCheckSucceed(vkCreateDevice(physicalDevice.m_Handle, &deviceCI, nullptr, &m_Device));

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

		return true;
	}

	bool RenderDevice::CreateGlobalMemoryAllocator()
	{
		VmaAllocatorCreateInfo allocatorCI = {};
		allocatorCI.instance = m_Inst;
		allocatorCI.physicalDevice = m_PhysicalDevices[m_PickedPhysicalDeviceIndex].m_Handle;
		allocatorCI.device = m_Device;
		allocatorCI.vulkanApiVersion = gVulkanVersion;

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
		vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
		vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
		vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
		vulkanFunctions.vkCreateImage = vkCreateImage;
		vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
		vulkanFunctions.vkDestroyImage = vkDestroyImage;
		vulkanFunctions.vkFreeMemory = vkFreeMemory;
		vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
		vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
		vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		vulkanFunctions.vkMapMemory = vkMapMemory;
		vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
		vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
		vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
		vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

		allocatorCI.pVulkanFunctions = &vulkanFunctions;
		allocatorCI.pAllocationCallbacks = nullptr;

		VulkanCheckSucceed(vmaCreateAllocator(&allocatorCI, &m_GlobalAllocator));

		return true;
	}

	bool RenderDevice::CollectInstanceLayerProps(const std::vector<const char*>& requiredLayers)
	{
		uint32_t layerCount = 0;
		VulkanCheckSucceed(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		ZE_ASSERT(layerCount > 0);

		std::vector<VkLayerProperties> layerProps(layerCount);
		VulkanCheckSucceed(vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data()));
		m_InstProps.m_LayerPropArray = layerProps;

		for (auto const& required : requiredLayers)
		{
			bool bFoundLayer = false;

			for (auto const& layer : layerProps)
			{
				if (std::string_view(required) == std::string_view(layer.layerName))
				{
					bFoundLayer = true;
					break;
				}
			}

			if (!bFoundLayer)
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

		ZE_ASSERT(extCount > 0);

		std::vector<VkExtensionProperties> extProps(extCount);
		VulkanCheckSucceed(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProps.data()));
		m_InstProps.m_ExtensionPropArray = extProps;

		for (auto const& required : requiredExtensions)
		{
			bool foundExt = false;

			for (auto const& ext : extProps)
			{
				if (std::string_view(required) == std::string_view(ext.extensionName))
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
		VulkanCheckSucceed(vkEnumerateDeviceLayerProperties(physicalDevice.m_Handle, &layerCount, nullptr));

		ZE_ASSERT(layerCount > 0);

		std::vector<VkLayerProperties> layerProps(layerCount);
		VulkanCheckSucceed(vkEnumerateDeviceLayerProperties(physicalDevice.m_Handle, &layerCount, layerProps.data()));
		m_DeviceProps.m_LayerPropArray = layerProps;

		for (auto const& required : requiredLayers)
		{
			auto bFoundLayer = false;

			for (auto const& layer : layerProps)
			{
				if (std::string_view(required) == std::string_view(layer.layerName))
				{
					bFoundLayer = true;
					break;
				}
			}

			if (!bFoundLayer)
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
		VulkanCheckSucceed(vkEnumerateDeviceExtensionProperties(physicalDevice.m_Handle, nullptr, &extCount, nullptr));

		ZE_ASSERT(extCount > 0);

		std::vector<VkExtensionProperties> extProps(extCount);
		VulkanCheckSucceed(vkEnumerateDeviceExtensionProperties(physicalDevice.m_Handle, nullptr, &extCount, extProps.data()));
		m_DeviceProps.m_ExtensionPropArray = extProps;

		for (auto const& required : requiredExtensions)
		{
			bool foundExt = false;

			for (auto const& ext : extProps)
			{
				if (std::string_view(required) == std::string_view(ext.extensionName))
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

	// void RenderDevice::ReleaseSubmittedCommandList(VkFence fence)
	// {
	// 	if (auto iter = m_SubmittedCommands.find(fence); iter != m_SubmittedCommands.end())
	// 	{
	// 		m_SubmittedCommands.erase(iter);
	// 	}
	// }

	bool RenderDevice::Initialize()
	{
		if (!CreateInstance())
		{
			return false;
		}

		if (!CreateDebugLayer())
		{
			return false;
		}
		
		if (!CreateWin32Surface())
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

		if (!CreateGlobalMemoryAllocator())
		{
			return false;
		}

		for (auto& cmdList : m_FrameCommandLists)
		{
			cmdList = new RenderCommandList(*this);
		}

		for (auto& cache : m_FrameDescriptorCaches)
		{
			cache = new DescriptorCache(*this);
		}
		
		return true;
	}

	void RenderDevice::Shutdown()
	{
		vkDeviceWaitIdle(m_Device);
		
		for (auto& cache : m_FrameDescriptorCaches)
		{
			delete cache;
			cache = nullptr;
		}
		
		for (auto& pCmdList : m_FrameCommandLists)
		{
			delete pCmdList;
			pCmdList = nullptr;
		}

		for (auto& queue : m_FrameDeferReleaseQueues)
		{
			queue.ReleaseAllImmediately();
		}
		
		if (m_GlobalAllocator)
		{
			vmaDestroyAllocator(m_GlobalAllocator);
		}

		if (m_Device)
		{
			vkDestroyDevice(m_Device, nullptr);
		}

		if (m_DebugUtilsMessenger)
		{
			auto* vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Inst, "vkDestroyDebugUtilsMessengerEXT"));
			ZE_ASSERT(vkDestroyDebugUtilsMessengerEXT);
			vkDestroyDebugUtilsMessengerEXT(m_Inst, m_DebugUtilsMessenger, nullptr);
		}

		if (m_Surface)
		{
			vkDestroySurfaceKHR(m_Inst, m_Surface, nullptr);
		}

		if (m_Inst)
		{
			vkDestroyInstance(m_Inst, nullptr);
		}
	}

	std::shared_ptr<RenderCommandList> RenderDevice::GetImmediateCommandList()
	{
		return std::make_shared<RenderCommandList>(*this);
	}

	void RenderDevice::SubmitCommandListAndWaitUntilFinish(RenderCommandList* pCmdList) const
	{
		VulkanZeroStruct(VkSubmitInfo, submitInfo);
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers = &pCmdList->m_CommandBuffer;
		submitInfo.commandBufferCount = 1;
		
		VulkanZeroStruct(VkFenceCreateInfo, fenceCI);
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		
		VkFence fence;
		VulkanCheckSucceed(vkCreateFence(m_Device, &fenceCI, nullptr, &fence));

		VulkanCheckSucceed(vkQueueSubmit(m_GraphicQueue, 1, &submitInfo, fence));
		VulkanCheckSucceed(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, kInfiniteWaitTime));
		vkDestroyFence(m_Device, fence, nullptr);
	}
	
	void RenderDevice::SubmitCommandList(RenderCommandList* pCmdList, const RenderWindow& renderWindow) const
	{
		constexpr VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VulkanZeroStruct(VkSubmitInfo, submitInfo);
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers = &pCmdList->m_CommandBuffer;
		submitInfo.commandBufferCount = 1u;
		submitInfo.pWaitSemaphores = &renderWindow.m_PresentCompleteSemaphores[m_FrameIndex];
		submitInfo.waitSemaphoreCount = 1u;
		submitInfo.pSignalSemaphores = &renderWindow.m_RenderCompleteSemaphores[m_FrameIndex];
		submitInfo.signalSemaphoreCount = 1u;
		submitInfo.pWaitDstStageMask = &waitStageMask;
		
		VulkanCheckSucceed(vkQueueSubmit(m_GraphicQueue, 1, &submitInfo, renderWindow.m_Fences[m_FrameIndex]));
	}

	void RenderDevice::DeferRelease(IDeferReleaseResource* pDeferReleaseResource)
	{
		ZE_ASSERT(m_HadBeganFrame);
		m_FrameDeferReleaseQueues[m_FrameIndex].DeferRelease(pDeferReleaseResource);
	}
	
	void RenderDevice::DeferRelease(const DeferReleaseLifetimeResource<Buffer>& deferReleaseResource)
	{
		ZE_ASSERT(m_HadBeganFrame);
		m_FrameDeferReleaseQueues[m_FrameIndex].DeferRelease(deferReleaseResource);
	}
	
	void RenderDevice::DeferRelease(const DeferReleaseLifetimeResource<Texture>& deferReleaseResource)
	{
		ZE_ASSERT(m_HadBeganFrame);
		m_FrameDeferReleaseQueues[m_FrameIndex].DeferRelease(deferReleaseResource);
	}

	inline const std::shared_ptr<Texture>& RenderDevice::GetSwapchainRenderTarget() const
	{
		return m_RenderModule.get().GetMainRenderWindow()->GetFrameSwapchainRenderTarget();
	}

	void RenderDevice::WaitUntilIdle() const
	{
		vkDeviceWaitIdle(m_Device);
	}
	
	void RenderDevice::BeginFrame()
	{
		ZE_ASSERT(!m_HadBeganFrame);
		m_FrameDeferReleaseQueues[m_FrameIndex].ReleaseAllImmediately();
		m_FrameCommandLists[m_FrameIndex]->Reset();
		m_HadBeganFrame = true;
	}
	
	void RenderDevice::EndFrame()
	{
		ZE_ASSERT(m_HadBeganFrame);
		m_FrameIndex = (m_FrameIndex + 1) % kSwapBufferCount;
		m_HadBeganFrame = false;
	}
}
