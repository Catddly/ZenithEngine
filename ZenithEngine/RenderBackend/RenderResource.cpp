#include "RenderResource.h"

#include "Core/Assertion.h"
#include "RenderDevice.h"
#include "RenderCommandList.h"

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

	Buffer::MappedMemoryScope::MappedMemoryScope(const std::shared_ptr<Buffer>& pBuffer)
		: m_pBuffer(pBuffer)
	{
		VulkanCheckSucceed(vmaMapMemory(m_pBuffer->m_RenderDevice.m_GlobalAllocator, m_pBuffer->m_Allocation, &m_pMappedMemory));
	}
	Buffer::MappedMemoryScope::~MappedMemoryScope()
	{
		vmaUnmapMemory(m_pBuffer->m_RenderDevice.m_GlobalAllocator, m_pBuffer->m_Allocation);
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

		//if (createDesc.m_bufferUploadData.HasValidData() && createDesc.m_bufferUploadData.CanBeUsedBy(createDesc))
		//{
		//	bufferCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		//}

		VmaAllocationCreateInfo vmaAllocationCI = {};
		vmaAllocationCI.usage = ToVmaMemoryUsage(desc.m_MemoryUsage);
		//if (createDesc.m_memoryFlag.IsFlagSet(RHI::ERenderResourceMemoryFlag::DedicatedMemory))
		//	vmaAllocationCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		//if (createDesc.m_memoryFlag.IsFlagSet(RHI::ERenderResourceMemoryFlag::PersistentMapping))
		//	vmaAllocationCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocInfo;
		VulkanCheckSucceed(vmaCreateBuffer(pBuffer->m_RenderDevice.m_GlobalAllocator, &bufferCI, &vmaAllocationCI, &pBuffer->m_Handle, &pBuffer->m_Allocation, &allocInfo));

		pBuffer->m_AllocatedSizeInByte = static_cast<uint32_t>(allocInfo.size);
		//if (createDesc.m_memoryFlag.IsFlagSet(RHI::ERenderResourceMemoryFlag::PersistentMapping))
		//{
		//	pVkBuffer->m_pMappedMemory = allocInfo.pMappedData;
		//}

		//pVkBuffer->m_desc.CopyIgnoreUploadData(createDesc);
		//pVkBuffer->m_desc.m_allocatedSize = allcatedMemorySize;
		//if (createDesc.m_bufferUploadData.HasValidData() && createDesc.m_bufferUploadData.CanBeUsedBy(createDesc))
		//{
		//	pVkBuffer->m_desc.m_usage.SetFlag(RHI::EBufferUsage::TransferDst);
		//	ImmediateUploadBufferData(pVkBuffer, createDesc.m_bufferUploadData);
		//}	

		return pBuffer;
	}

	Buffer* Buffer::Create(RenderDevice& renderDevice, const BufferDesc& desc, const void* pUploadData, uint32_t uploadDataSizeInByte)
	{
		if (!desc.IsValid())
		{
			return nullptr;
		}

		auto* pBuffer = new Buffer(renderDevice, desc);

		BufferDesc stagingBufferDesc{
			.m_Size = desc.m_Size,
			.m_Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.m_MemoryUsage = BufferMemoryUsage::CpuToGpu,
		};

		auto pStagingBuffer = std::make_shared<Buffer>(renderDevice, stagingBufferDesc);

		// copy to staging buffer
		{
			auto mappedMemory = pStagingBuffer->Map();
			memcpy(mappedMemory.m_pMappedMemory, pUploadData, uploadDataSizeInByte);
		}

		// copy to VRAM
		{
			auto pCmdList = pBuffer->m_RenderDevice.GetImmediateCommandList();
			pCmdList->BeginRecord();
			pCmdList->CmdCopyBuffer(pStagingBuffer.get(), pBuffer);
			pCmdList->EndRecord();

			auto submittedCmd = pBuffer->m_RenderDevice.SubmitCommandList(std::move(pCmdList));
			submittedCmd.WaitUntilFinished();
		}

		return pBuffer;
	}

	Buffer::Buffer(RenderDevice& renderDevice, const BufferDesc& desc)
		: m_RenderDevice(renderDevice), m_Desc(desc)
	{
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(m_RenderDevice.m_GlobalAllocator, m_Handle, nullptr);
		m_Handle = nullptr;
	}

	Buffer::MappedMemoryScope Buffer::Map()
	{
		return MappedMemoryScope(shared_from_this());
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
		imageCI.usage = desc.m_bIsDepthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocCI = {};
		allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VmaAllocationInfo allocationInfo;
		VulkanCheckSucceed(vmaCreateImage(pTex->m_RenderDevice.m_GlobalAllocator, &imageCI, &allocCI, &pTex->m_Handle, &pTex->m_Allocation, &allocationInfo));

		pTex->m_AllocatedSizeInByte = static_cast<uint32_t>(allocationInfo.size);

		return pTex;
	}

	Texture::Texture(RenderDevice& renderDevice, const TextureDesc& desc)
		: m_RenderDevice(renderDevice), m_Desc(desc)
	{}

	Texture::~Texture()
	{
		vmaDestroyImage(m_RenderDevice.m_GlobalAllocator, m_Handle, m_Allocation);
		m_Handle = nullptr;
	}
}
