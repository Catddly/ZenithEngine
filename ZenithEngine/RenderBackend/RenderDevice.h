#pragma once

#include "IRenderDevice.h"
#include "Core/Assertion.h"
#include "RenderBackend/RenderResource.h"
#include "RenderBackend/DeferReleaseQueue.h"

#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

#include <vector>
#include <limits>
#include <memory>
#include <functional>

namespace ZE::Render { class RenderModule; }

namespace ZE::RenderBackend
{
	class RenderCommandList;
	class DescriptorCache;

	class RenderDevice : public IRenderDevice
	{
	public:

		static constexpr uint32_t kSwapBufferCount = 2u;
		static constexpr uint64_t kInfiniteWaitTime = std::numeric_limits<uint64_t>::max();

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
			VkPhysicalDevice					m_Handle;

			std::vector<QueueFamily>			m_QueueArray;

			uint32_t							m_PickScore = 0u;

			VkPhysicalDeviceFeatures			m_Features;
			VkPhysicalDeviceProperties			m_Props;
			VkPhysicalDeviceMemoryProperties	m_MemoryProps;
		};

		RenderDevice(Render::RenderModule& renderModule, const Settings& settings);

		virtual bool Initialize() override;
		virtual void Shutdown() override;

		std::shared_ptr<RenderCommandList> GetImmediateCommandList();

		void SubmitCommandList(RenderCommandList* pCmdList, const RenderWindow& renderWindow) const;
		void SubmitCommandListAndWaitUntilFinish(RenderCommandList* pCmdList) const;

		void DeferRelease(IDeferReleaseResource* pDeferReleaseResource);
		void DeferRelease(const DeferReleaseLifetimeResource<Buffer>& deferReleaseResource);
		void DeferRelease(const DeferReleaseLifetimeResource<Texture>& deferReleaseResource);

		inline const std::shared_ptr<Texture>& GetSwapchainRenderTarget() const;

		uint32_t GetFrameIndex() const { return m_FrameIndex; }
		RenderCommandList* GetFrameCommandList() const { return m_FrameCommandLists[m_FrameIndex]; }
		DescriptorCache* GetFrameDescriptorCache() const { return m_FrameDescriptorCaches[m_FrameIndex]; }
		VkDevice GetNativeDevice() const { return m_Device; }

		void WaitUntilIdle() const;
		
		void BeginFrame();
		void EndFrame();

	protected:

		const PhysicalDevice& GetPhysicalDevice() const { ZE_CHECK(m_PickedPhysicalDeviceIndex != -1); return m_PhysicalDevices[m_PickedPhysicalDeviceIndex]; }
	
	private:

		bool CreateInstance();
		bool CreateDebugLayer();
		bool CreateWin32Surface();
		bool PickPhysicalDevice();
		bool CreateDevice();
		bool CreateGlobalMemoryAllocator();

		bool CollectInstanceLayerProps(const std::vector<const char*>& requiredLayers);
		bool CollectInstanceExtensionProps(const std::vector<const char*>& requiredExtensions);

		bool CollectDeviceLayerProps(const std::vector<const char*>& requiredLayers);
		bool CollectDeviceExtensionProps(const std::vector<const char*>& requiredExtensions);

	private:

		friend class RenderWindow;
		friend class RenderCommandList;
		friend class PipelineState;
		friend class GraphicPipelineState;

		friend class Buffer;
		friend class Texture;
		
		friend struct SubmittedCommandHandle;

	private:

		std::reference_wrapper<Render::RenderModule>	m_RenderModule;
		Settings										m_Settings;

		VkInstance										m_Inst = nullptr;
		InstanceProperties								m_InstProps;

		VkDebugUtilsMessengerEXT						m_DebugUtilsMessenger = nullptr;

		VkSurfaceKHR									m_Surface = nullptr;

		std::vector<PhysicalDevice>						m_PhysicalDevices;
		int32_t											m_PickedPhysicalDeviceIndex = -1;

		VkDevice										m_Device = nullptr;
		DeviceProperties								m_DeviceProps;
														
		VmaAllocator									m_GlobalAllocator = nullptr;
														
		VkQueue											m_GraphicQueue = nullptr;
		uint32_t										m_GraphicQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkQueue											m_ComputeQueue = nullptr;
		uint32_t										m_ComputeQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
		VkQueue											m_TransferQueue = nullptr;
		uint32_t										m_TransferQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
			
		uint32_t												m_FrameIndex = 0;
		std::array<RenderCommandList*, kSwapBufferCount>		m_FrameCommandLists = {};

		std::array<DescriptorCache*, kSwapBufferCount>			m_FrameDescriptorCaches = {};
		std::array<DeferReleaseQueue, kSwapBufferCount>			m_FrameDeferReleaseQueues = {};

		bool													m_HadBeganFrame = false;
	};
}
