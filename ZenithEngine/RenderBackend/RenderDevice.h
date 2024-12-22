#pragma once

#include "IRenderDevice.h"
#include "Core/Assertion.h"

#include "vulkan/vulkan_core.h"

#include <vector>
#include <limits>

namespace ZE::Render { class RenderModule; }

namespace ZE::RenderBackend
{
	class RenderDevice : public IRenderDevice
	{
		friend class RenderWindow;
		friend class RenderCommandList;

	public:

		static constexpr uint32_t kSwapBufferCount = 2;

		struct Settings
		{
			bool									m_EnableValidationLayer = true;
		};

		struct InstanceProperties
		{
			std::vector<VkLayerProperties>			m_LayerPropArray;
			std::vector<VkExtensionProperties>		m_ExtensionPropArray;
		};

		struct DeviceProperties
		{
			std::vector<VkLayerProperties>			m_LayerPropArray;
			std::vector<VkExtensionProperties>		m_ExtensionPropArray;
		};

		struct QueueFamily
		{
			uint32_t							m_Index = std::numeric_limits<uint32_t>::max();
			VkQueueFamilyProperties				m_Props;

			inline bool IsGraphicQueue() const
			{
				return (m_Props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			}

			inline bool IsTransferQueue() const
			{
				return (m_Props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
			}

			inline bool IsComputeQueue() const
			{
				return (m_Props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
			}
		};

		struct PhysicalDevice
		{
			VkPhysicalDevice					m_pHandle;

			std::vector<QueueFamily>			m_QueueArray;

			uint32_t							m_PickScore = 0;

			VkPhysicalDeviceFeatures			m_Features;
			VkPhysicalDeviceProperties			m_Props;
			VkPhysicalDeviceMemoryProperties	m_MemoryProps;
		};

		RenderDevice(Render::RenderModule& renderModule, const Settings& settings);

		virtual bool Initialize() override;
		virtual void Shutdown() override;

	protected:

		const PhysicalDevice& GetPhysicalDevice() const { ZE_CHECK(m_PickedPhysicalDeviceIndex != -1); return m_PhysicalDevices[m_PickedPhysicalDeviceIndex]; };
	private:

		bool CreateInstance();
		bool CreateDebugLayer();
		bool CreateWin32Surface();
		bool PickPhysicalDevice();
		bool CreateDevice();

		bool CollectInstanceLayerProps(const std::vector<const char*>& requiredLayers);
		bool CollectInstanceExtensionProps(const std::vector<const char*>& requiredExtensions);

		bool CollectDeviceLayerProps(const std::vector<const char*>& requiredLayers);
		bool CollectDeviceExtensionProps(const std::vector<const char*>& requiredExtensions);

	private:

		Render::RenderModule&			m_RenderModule;
		Settings						m_Settings;

		VkInstance						m_Inst = nullptr;
		InstanceProperties				m_InstProps;

		VkDebugUtilsMessengerEXT		m_DebugUtilsMessenger = nullptr;

		VkSurfaceKHR					m_Surface = nullptr;

		std::vector<PhysicalDevice>		m_PhysicalDevices;
		int32_t							m_PickedPhysicalDeviceIndex = -1;

		VkDevice						m_Device = nullptr;
		DeviceProperties				m_DeviceProps;

		VkQueue							m_GraphicQueue = nullptr;
		uint32_t						m_GraphicQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkQueue							m_ComputeQueue = nullptr;
		uint32_t						m_ComputeQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkQueue							m_TransferQueue = nullptr;
		uint32_t						m_TransferQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
	};
}