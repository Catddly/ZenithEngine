#pragma once

#include "RenderDeviceChild.h"
#include "DeferReleaseQueue.h"

#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <string_view>

namespace ZE::RenderBackend
{
	class RenderDevice;

	enum class BufferMemoryUsage
	{
		CpuToGpu,
		GpuOnly
	};

	struct BufferDesc
	{
		BufferDesc() = default;
		BufferDesc(std::string_view name)
			: m_DebugName(name)
		{}
		
		uint32_t					m_Size = 0;

		VkBufferUsageFlags			m_Usage = 0;
		BufferMemoryUsage			m_MemoryUsage = BufferMemoryUsage::CpuToGpu;

		// TODO: debug build only
		std::string					m_DebugName = "Unknown";
		
		inline bool IsValid() const { return m_Size != 0; }
	};

	class Buffer : public std::enable_shared_from_this<Buffer>, public RenderDeviceChild
	{
		friend class RenderDevice;

		friend struct MappedMemoryScope;

	public:

		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc);
		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc, const void* pUploadData, uint32_t uploadDataSizeInByte);
		template <typename T>
		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc, const T& uploadData);

		virtual ~Buffer();

		struct MappedMemoryScope final
		{
			void* m_pMappedMemory = nullptr;

			MappedMemoryScope(const std::shared_ptr<Buffer>& pBuffer);
			~MappedMemoryScope();

			template <typename T>
			T* MapAs()
			{
				return reinterpret_cast<T*>(m_pMappedMemory);
			}

		private:

			std::shared_ptr<Buffer>				m_pBuffer = nullptr;
		};

		MappedMemoryScope Map();

		const BufferDesc& GetDesc() const { return m_Desc; }
		VkBuffer GetNativeHandle() const { return m_Handle; }

	private:

		Buffer(RenderDevice& renderDevice, const BufferDesc& desc);
		
	private:
		
		BufferDesc				m_Desc;
		VkBuffer				m_Handle = nullptr;

		VmaAllocation			m_Allocation = nullptr;
		uint32_t				m_AllocatedSizeInByte = 0;
	};

	template <typename T>
	Buffer* Buffer::Create(RenderDevice& renderDevice, const BufferDesc& desc, const T& uploadData)
	{
		return Create(renderDevice, desc, &uploadData, sizeof(T));
	}

	enum class ETextureUsage : uint8_t
	{
		TransferSrc = 0,
		TransferDst,
		Sampled,
		Storage,
		Color,
		DepthStencil,
		Transient,
		Input,
	};
	
	struct TextureDesc
	{
		TextureDesc() = default;
		TextureDesc(std::string_view name)
			: m_DebugName(name)
		{}
		
		glm::uvec2				m_Size{ 0u, 0u };
		VkFormat				m_Format = VK_FORMAT_UNDEFINED;
		// TODO: enum bit flags
		uint8_t					m_Usage = 0;
		uint16_t				m_MipCount = 1u;
		
		// TODO: debug build only
		std::string					m_DebugName = "Unknown";

		bool IsValid() const { return m_Size.x != 0 && m_Size.y != 0 && m_Format != VK_FORMAT_UNDEFINED; }
	};

	VkFlags SpeculateVkImageAspectFlagsFromDesc(const TextureDesc& desc);
	VkImageLayout SpeculateVkImageLayoutFromDesc(const TextureDesc& desc);
	
	class Texture : public std::enable_shared_from_this<Texture>, public RenderDeviceChild
	{
		friend class RenderWindow;
		friend class RenderDevice;

	public:

		static Texture* Create(RenderDevice& renderDevice, const TextureDesc& desc);

		virtual ~Texture();

		const TextureDesc& GetDesc() const { return m_Desc; }
		VkImage GetNativeHandle() const { return m_Handle; }

		VkImageView GetOrCreateView();

	private:
		
		Texture(RenderDevice& renderDevice, const TextureDesc& desc);
	
	private:

		TextureDesc				m_Desc;
		
		VkImage					m_Handle = nullptr;

		VmaAllocation			m_Allocation;
		uint32_t				m_AllocatedSizeInByte = 0;

		// TODO: multi-view cache
		VkImageView				m_View = nullptr;
	};

	enum class ERenderPassOperation : uint8_t
	{
		Load = 0,
		Store,
		DontCare
	};
}
