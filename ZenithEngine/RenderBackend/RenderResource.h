#pragma once

#include "VulkanHelper.h"

#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>
#include <glm/vec2.hpp>

#include <memory>

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
		uint32_t					m_Size = 0;

		VkBufferUsageFlags			m_Usage = 0;
		BufferMemoryUsage			m_MemoryUsage = BufferMemoryUsage::CpuToGpu;

		inline bool IsValid() const { return m_Size != 0; }
	};

	class Buffer : public std::enable_shared_from_this<Buffer>
	{
		friend class RenderDevice;
		friend class RenderCommandList;

		friend struct MappedMemoryScope;

	public:

		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc);
		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc, const void* pUploadData, uint32_t uploadDataSizeInByte);
		template <typename T>
		static Buffer* Create(RenderDevice& renderDevice, const BufferDesc& desc, const T& uploadData);

		Buffer(RenderDevice& renderDevice, const BufferDesc& desc);
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

	private:

		RenderDevice&			m_RenderDevice;

		BufferDesc				m_Desc;
		VkBuffer				m_Handle = nullptr;

		VmaAllocation			m_Allocation;
		uint32_t				m_AllocatedSizeInByte = 0;
	};

	template <typename T>
	Buffer* Buffer::Create(RenderDevice& renderDevice, const BufferDesc& desc, const T& uploadData)
	{
		return Create(renderDevice, desc, &uploadData, sizeof(T));
	}

	struct TextureDesc
	{
		glm::uvec2				m_Size{ 0u, 0u };
		VkFormat				m_Format = VK_FORMAT_UNDEFINED;

		bool					m_bIsDepthStencil = false;

		inline bool IsValid() const { return m_Size.x != 0 && m_Size.y != 0 && m_Format != VK_FORMAT_UNDEFINED; }
	};

	class Texture : public std::enable_shared_from_this<Texture>
	{
		friend class RenderWindow;
		friend class RenderDevice;

	public:

		static Texture* Create(RenderDevice& renderDevice, const TextureDesc& desc);

		Texture(RenderDevice& renderDevice, const TextureDesc& desc);
		virtual ~Texture();

		const TextureDesc& GetDesc() const { return m_Desc; }

	private:

		RenderDevice&			m_RenderDevice;

		TextureDesc				m_Desc;
		VkImage					m_Handle = nullptr;

		VmaAllocation			m_Allocation;
		uint32_t				m_AllocatedSizeInByte = 0;
	};
}
