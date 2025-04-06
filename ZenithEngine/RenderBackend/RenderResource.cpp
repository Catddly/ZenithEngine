#include "RenderResource.h"

#include "Core/Assertion.h"
#include "RenderDevice.h"
#include "RenderCommandList.h"
#include "VulkanHelper.h"

namespace ZE::RenderBackend
{
	static VmaMemoryUsage ToVmaMemoryUsage(BufferMemoryUsage usage)
	{
		switch (usage)
		{
		case ZE::RenderBackend::BufferMemoryUsage::CpuToGpu: return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case ZE::RenderBackend::BufferMemoryUsage::GpuOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
		default:
			break;
		}

		ZE_CHECK(false);
		return VMA_MEMORY_USAGE_UNKNOWN;
	}

	static VkFlags ToVkImageUsageFlags(uint8_t usage)
	{
		VkFlags flags = 0;
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::TransferSrc))) != 0)
		{
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::TransferDst))) != 0)
		{
			flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::Sampled))) != 0)
		{
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::Storage))) != 0)
		{
			flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::Color))) != 0)
		{
			flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::DepthStencil))) != 0)
		{
			flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::Transient))) != 0)
		{
			flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
		if ((usage & (1 << static_cast<uint8_t>(ETextureUsage::Input))) != 0)
		{
			flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		}
		return flags;
	}

	Buffer::MappedMemoryScope::MappedMemoryScope(const std::shared_ptr<Buffer>& pBuffer)
		: m_pBuffer(pBuffer)
	{
		VulkanCheckSucceed(vmaMapMemory(m_pBuffer->GetRenderDevice().m_GlobalAllocator, m_pBuffer->m_Allocation, &m_pMappedMemory));
	}
	Buffer::MappedMemoryScope::~MappedMemoryScope()
	{
		vmaUnmapMemory(m_pBuffer->GetRenderDevice().m_GlobalAllocator, m_pBuffer->m_Allocation);
	}

	Buffer* Buffer::Create(RenderDevice& renderDevice, const BufferDesc& desc)
	{
		if (!desc.IsValid())
		{
			return nullptr;
		}

		auto* pBuffer = new Buffer(renderDevice, desc);
		if (!pBuffer)
		{
			return nullptr;
		}

		VkDeviceSize bufferSize = static_cast<VkDeviceSize>(desc.m_Size);

		VulkanZeroStruct(VkBufferCreateInfo, bufferCI);
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.size = bufferSize;
		bufferCI.usage = desc.m_Usage;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		VmaAllocationCreateInfo vmaAllocationCI = {};
		vmaAllocationCI.usage = ToVmaMemoryUsage(desc.m_MemoryUsage);

		VmaAllocationInfo allocInfo;
		VulkanCheckSucceed(vmaCreateBuffer(renderDevice.m_GlobalAllocator, &bufferCI, &vmaAllocationCI, &pBuffer->m_Handle, &pBuffer->m_Allocation, &allocInfo));

		// TODO: debug build only
		vmaSetAllocationName(renderDevice.m_GlobalAllocator, pBuffer->m_Allocation, desc.m_DebugName.c_str());

		// VulkanZeroStruct(VkDebugUtilsObjectNameInfoEXT, debugNameInfo);
		// debugNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		// debugNameInfo.pObjectName = desc.m_DebugName.c_str();
		// vkSetDebugUtilsObjectNameEXT(renderDevice.GetNativeDevice(), &debugNameInfo);
		
		pBuffer->m_AllocatedSizeInByte = static_cast<uint32_t>(allocInfo.size);

		return pBuffer;
	}

	Buffer* Buffer::Create(RenderDevice& renderDevice, const BufferDesc& desc, const void* pUploadData, uint32_t uploadDataSizeInByte)
	{
		if (!desc.IsValid())
		{
			return nullptr;
		}

		auto* pBuffer = Create(renderDevice, desc);
		if (!pBuffer)
		{
			return nullptr;
		}

		BufferDesc stagingBufferDesc("staging buffer");
		stagingBufferDesc.m_Size = desc.m_Size;
		stagingBufferDesc.m_Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferDesc.m_MemoryUsage = BufferMemoryUsage::CpuToGpu;
		
		auto pStagingBuffer = std::shared_ptr<Buffer>(Create(renderDevice, stagingBufferDesc));

		// copy to staging buffer
		{
			auto mappedMemory = pStagingBuffer->Map();
			memcpy(mappedMemory.m_pMappedMemory, pUploadData, uploadDataSizeInByte);
		}

		// copy to VRAM
		{
			auto pCmdList = pBuffer->GetRenderDevice().GetImmediateCommandList();
			pCmdList->BeginRecord();
			pCmdList->CmdCopyBuffer(pStagingBuffer.get(), pBuffer);
			pCmdList->EndRecord();

			pBuffer->GetRenderDevice().SubmitCommandListAndWaitUntilFinish(pCmdList.get());
		}

		return pBuffer;
	}

	Buffer::Buffer(RenderDevice& renderDevice, const BufferDesc& desc)
		: m_Desc(desc)
	{
		SetRenderDevice(&renderDevice);
	}
	
	Buffer::~Buffer()
	{
		vmaDestroyBuffer(GetRenderDevice().m_GlobalAllocator, m_Handle, m_Allocation);
		m_Handle = nullptr;
	}

	Buffer::MappedMemoryScope Buffer::Map()
	{
		return {shared_from_this()};
	}

	VkFlags SpeculateVkImageAspectFlagsFromDesc(const TextureDesc& desc)
	{
		if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::Color))) != 0)
		{
			if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::DepthStencil))) != 0)
			{
				ZE_LOG_ERROR("Conflict texture usage (color with depth stencil)!");
				return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
			}

			return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::DepthStencil))) != 0)
		{
			if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::Color))) != 0)
			{
				ZE_LOG_ERROR("Conflict texture usage (depth stencil with color)!");
				return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
			}

			switch ( desc.m_Format )
			{
				case VK_FORMAT_D16_UNORM: return VK_IMAGE_ASPECT_DEPTH_BIT;
				case VK_FORMAT_X8_D24_UNORM_PACK32: return VK_IMAGE_ASPECT_DEPTH_BIT;
				case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
					
				case VK_FORMAT_D16_UNORM_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				case VK_FORMAT_D24_UNORM_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

				default: return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
			}
		}
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageLayout SpeculateVkImageLayoutFromDesc(const TextureDesc& desc)
	{
		if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::Color))) != 0)
		{
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if ((desc.m_Usage & (1 << static_cast<uint8_t>(RenderBackend::ETextureUsage::DepthStencil))) != 0)
		{
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}

	Texture* Texture::Create(RenderDevice& renderDevice, const TextureDesc& desc)
	{
		if (!desc.IsValid())
		{
			return nullptr;
		}

		auto* pTex = new Texture(renderDevice, desc);

		VulkanZeroStruct(VkImageCreateInfo, imageCI);
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = desc.m_Format;
		imageCI.extent = { desc.m_Size[0], desc.m_Size[1], 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = ToVkImageUsageFlags(desc.m_Usage);
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocCI = {};
		allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VmaAllocationInfo allocationInfo;
		VulkanCheckSucceed(vmaCreateImage(pTex->GetRenderDevice().m_GlobalAllocator, &imageCI, &allocCI, &pTex->m_Handle, &pTex->m_Allocation, &allocationInfo));

		// TODO: debug build only
		vmaSetAllocationName(pTex->GetRenderDevice().m_GlobalAllocator, pTex->m_Allocation, desc.m_DebugName.c_str());
		
		pTex->m_AllocatedSizeInByte = static_cast<uint32_t>(allocationInfo.size);

		return pTex;
	}

	VkImageView Texture::GetOrCreateView()
	{
		if (m_View)
		{
			return m_View;
		}
		
		VulkanZeroStruct(VkImageViewCreateInfo, createInfo);
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.format = m_Desc.m_Format;
		createInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		createInfo.subresourceRange.aspectMask = SpeculateVkImageAspectFlagsFromDesc(m_Desc);
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = m_Desc.m_MipCount;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.image = m_Handle;
		
		VulkanCheckSucceed(vkCreateImageView(GetRenderDevice().GetNativeDevice(), &createInfo, nullptr, &m_View));
		return m_View;
	}
	
	Texture::Texture(RenderDevice& renderDevice, const TextureDesc& desc)
		: m_Desc(desc)
	{
		SetRenderDevice(&renderDevice);
	}

	Texture::~Texture()
	{
		if (m_View)
		{
			vkDestroyImageView(GetRenderDevice().GetNativeDevice(), m_View, nullptr);
			m_View = nullptr;
		}

		if (m_Handle)
		{
			vmaDestroyImage(GetRenderDevice().m_GlobalAllocator, m_Handle, m_Allocation);
			m_Handle = nullptr;	
		}
	}
}
