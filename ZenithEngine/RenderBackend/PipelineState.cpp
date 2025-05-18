#include "PipelineState.h"

#include "Log/Log.h"
#include "Core/Hash.h"
#include "RenderDevice.h"
#include "Render/Shader.h"
#include "VulkanHelper.h"
#include "DescriptorCache.h"

#include <refl.hpp>

#include <unordered_map>

namespace ZE::RenderBackend
{
	static VkDescriptorType ToVkDescriptorType(Render::EShaderBindingResourceType type)
	{
		switch (type)
		{
		case Render::EShaderBindingResourceType::Unknown: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		case Render::EShaderBindingResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Render::EShaderBindingResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case Render::EShaderBindingResourceType::Texture2D: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		default:
			break;
		}

		ZE_ASSERT(false);
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	static VkShaderStageFlagBits GetShaderVkType(const Render::Shader* pShader)
	{
		if (pShader->CanDowncastTo<Render::VertexShader>())
		{
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
		
		if (pShader->CanDowncastTo<Render::PixelShader>())
		{
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	PipelineState::PipelineState(RenderDevice& renderDevice)
	{
		SetRenderDevice(&renderDevice);
	}

	uint64_t PipelineStateCreateDesc::GetHash() const
	{
		uint64_t hash = 0;
		for (const auto& pShader : m_Shaders)
		{
			if (pShader)
			{
				hash ^= pShader->GetHash();
			}
		}
		return hash;
	}
	
	bool PipelineState::CreateInPlace(PipelineState* pPipelineState, const PipelineStateCreateDesc& CreateDesc)
	{
		std::unordered_map<Render::EShaderBindingResourceType, uint32_t> resourceCountMap;
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> setLayoutBindingArray;

		for (auto& pShader : CreateDesc.m_Shaders)
		{
			if (!pShader)
			{
				continue;
			}

			const auto& shaderSetMap = pShader->m_Layout.m_ResourceSetArray;
			pPipelineState->m_DescriptorSetLayouts.resize(pPipelineState->m_DescriptorSetLayouts.size() + shaderSetMap.size());
			setLayoutBindingArray.resize(setLayoutBindingArray.size() + shaderSetMap.size());
			
			for (uint32_t set = 0; set < shaderSetMap.size(); ++set)
			{
				const uint32_t prevBindingCount = static_cast<uint32_t>(setLayoutBindingArray[set].size());
				setLayoutBindingArray[set].resize(shaderSetMap[set].size());

				for (uint32_t i = 0; i < prevBindingCount; ++i)
				{
					setLayoutBindingArray[set][i].stageFlags |= GetShaderVkType(pShader);
				}

				pPipelineState->m_AllocatedSetBindingMap.reserve(pPipelineState->m_AllocatedSetBindingMap.size() + shaderSetMap[set].size());
				pPipelineState->m_AllocatedResourceTypeMap.reserve(pPipelineState->m_AllocatedResourceTypeMap.size() + shaderSetMap[set].size());

				// assume shader layout is consistent across multiple pipeline shaders
				uint32_t currentBinding = prevBindingCount;
				for (const auto& [name, type] : shaderSetMap[set])
				{
					setLayoutBindingArray[set][currentBinding].binding = currentBinding;
					setLayoutBindingArray[set][currentBinding].descriptorCount = 1;
					setLayoutBindingArray[set][currentBinding].pImmutableSamplers = nullptr;
					setLayoutBindingArray[set][currentBinding].stageFlags = GetShaderVkType(pShader);
					setLayoutBindingArray[set][currentBinding].descriptorType = ToVkDescriptorType(type);
					
					// TODO: sanity check
					pPipelineState->m_AllocatedSetBindingMap.emplace(name, std::make_pair(set, currentBinding));
					pPipelineState->m_AllocatedResourceTypeMap.emplace(name, type);

					resourceCountMap[type] += 1;
					
					currentBinding++;
				}
			}
		}

		for (uint32_t set = 0; set < setLayoutBindingArray.size(); ++set)
		{
			VulkanZeroStruct(VkDescriptorSetLayoutCreateInfo, setLayoutCI);
			setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindingArray[set].size());
			setLayoutCI.pBindings = setLayoutBindingArray[set].data();
			setLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VulkanCheckSucceed(vkCreateDescriptorSetLayout(pPipelineState->GetRenderDevice().GetNativeDevice(), &setLayoutCI, nullptr, &pPipelineState->m_DescriptorSetLayouts[set]));
		}

		std::vector<VkDescriptorPoolSize> poolSizeArray;
		poolSizeArray.reserve(resourceCountMap.size());
		for (const auto& [type, count] : resourceCountMap)
		{
			poolSizeArray.emplace_back(ToVkDescriptorType(type), count);
		}

		VulkanZeroStruct(VkDescriptorPoolCreateInfo, poolCreateInfo);
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = static_cast<uint32_t>(pPipelineState->m_DescriptorSetLayouts.size()) * RenderDevice::kSwapBufferCount;
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizeArray.size());
		poolCreateInfo.pPoolSizes = poolSizeArray.data();
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VulkanCheckSucceed(vkCreateDescriptorPool(pPipelineState->GetRenderDevice().GetNativeDevice(), &poolCreateInfo, nullptr, &pPipelineState->m_DescriptorPool));

		VulkanZeroStruct(VkPipelineLayoutCreateInfo, pipelineLayoutCI);
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(pPipelineState->m_DescriptorSetLayouts.size());
		pipelineLayoutCI.pSetLayouts = pPipelineState->m_DescriptorSetLayouts.data();

		VulkanCheckSucceed(vkCreatePipelineLayout(pPipelineState->GetRenderDevice().GetNativeDevice(), &pipelineLayoutCI, nullptr, &pPipelineState->m_Layout));

		return true;
	}
	
	PipelineState::BoundShaderResourceLocation PipelineState::FindBoundResourceLocation(const std::string& name)
	{
		if (auto iter = m_AllocatedSetBindingMap.find(name); iter != m_AllocatedSetBindingMap.end())
		{
			return {iter->second.first, iter->second.second};
		}

		return {};
	}
	
	Render::EShaderBindingResourceType PipelineState::FindBoundResourceType(const std::string& name)
	{
		if (auto iter = m_AllocatedResourceTypeMap.find(name); iter != m_AllocatedResourceTypeMap.end())
		{
			return iter->second;
		}

		return Render::EShaderBindingResourceType::Unknown;
	}

	PipelineState::~PipelineState()
	{
		for (auto& setLayout : m_DescriptorSetLayouts)
		{
			vkDestroyDescriptorSetLayout(GetRenderDevice().GetNativeDevice(), setLayout, nullptr);
		}
		m_DescriptorSetLayouts.clear();
		vkDestroyDescriptorPool(GetRenderDevice().GetNativeDevice(), m_DescriptorPool, nullptr);
		m_DescriptorPool = nullptr;
		vkDestroyPipelineLayout(GetRenderDevice().GetNativeDevice(), m_Layout, nullptr);
		m_Layout = nullptr;

		GetRenderDevice().GetFrameDescriptorCache()->MarkDirty(this);
	}

	// GraphicPipelineState* GraphicPipelineState::Builder::Build(RenderDevice& renderDevice)
	// {
	// 	if (!m_pVertexShader || !m_pVertexShader->m_Shader)
	// 	{
	// 		ZE_LOG_ERROR("Graphic pipeline must be created with a valid vertex shader!");
	// 		return nullptr;
	// 	}
	//
	// 	Settings settings{
	// 		.m_ColorInputFormatArray = std::move(m_ColorInputFormatArray),
	// 		.m_DepthStencilFormat = m_DepthStencilFormat,
	// 	};
	//
	// 	return Create(renderDevice, settings, m_pVertexShader, m_pPixelShader);
	// }

	GraphicPipelineState::GraphicPipelineState(RenderDevice& renderDevice)
		: PipelineState(renderDevice)
	{}

	GraphicPipelineState* GraphicPipelineState::Create(RenderDevice& renderDevice, const GraphicPipelineStateCreateDesc& CreateDesc)
	{
		// Must have at least vertex shader
		if (!CreateDesc.m_Shaders[0])
		{
			return nullptr;
		}

		GraphicPipelineState* pGraphicPSO = new GraphicPipelineState(renderDevice);
		pGraphicPSO->m_CreateDesc = CreateDesc;
		
		if (!CreateInPlace(pGraphicPSO, CreateDesc))
		{
			delete pGraphicPSO;
			return nullptr;
		}

		VulkanZeroStruct(VkPipelineInputAssemblyStateCreateInfo, inputAssemblyStateCI);
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VulkanZeroStruct(VkPipelineRasterizationStateCreateInfo, rasterizationStateCI);
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.rasterizerDiscardEnable = !CreateDesc.m_Shaders[1] ? VK_TRUE : VK_FALSE;
		rasterizationStateCI.depthBiasEnable = VK_FALSE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VulkanZeroStruct(VkPipelineColorBlendStateCreateInfo, blendStateCI);
		blendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendStateCI.attachmentCount = 1;
		blendStateCI.pAttachments = &blendAttachmentState;

		VulkanZeroStruct(VkPipelineViewportStateCreateInfo, viewportStateCI);
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.scissorCount = 1;
		viewportStateCI.viewportCount = 1;

		VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VulkanZeroStruct(VkPipelineDynamicStateCreateInfo, dynamicStateCI);
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables;
		dynamicStateCI.dynamicStateCount = 2u;

		VulkanZeroStruct(VkPipelineDepthStencilStateCreateInfo, depthStencilStateCI);
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;
		depthStencilStateCI.front = depthStencilStateCI.back;

		VulkanZeroStruct(VkPipelineMultisampleStateCreateInfo, multisampleStateCI);
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		const auto* pVertexShader = static_cast<const Render::VertexShader*>(CreateDesc.m_Shaders[0]);
		const auto* pPixelShader = static_cast<const Render::PixelShader*>(CreateDesc.m_Shaders[1]);
		
		VkVertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = pVertexShader->m_InputLayout.m_InputSizeInByte;

		VulkanZeroStruct(VkPipelineVertexInputStateCreateInfo, vertexInputStateCI);
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(pVertexShader->m_InputLayout.m_InputAttribArray.size());
		vertexInputStateCI.pVertexAttributeDescriptions = pVertexShader->m_InputLayout.m_InputAttribArray.data();

		std::vector<VkPipelineShaderStageCreateInfo> shaderPipelineInfos(pPixelShader ? 2 : 1);

		memset(shaderPipelineInfos.data(), 0, sizeof(VkPipelineShaderStageCreateInfo));
		shaderPipelineInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderPipelineInfos[0].module = pVertexShader->m_Shader;
		shaderPipelineInfos[0].pName = "Main";
		shaderPipelineInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

		if (pPixelShader)
		{
			memset(&shaderPipelineInfos[1], 0, sizeof(VkPipelineShaderStageCreateInfo));
			shaderPipelineInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderPipelineInfos[1].module = pPixelShader->m_Shader;
			shaderPipelineInfos[1].pName = "Main";
			shaderPipelineInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		// attachment information for dynamic rendering
		VulkanZeroStruct(VkPipelineRenderingCreateInfoKHR, pipelineRenderingCI);
		pipelineRenderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingCI.colorAttachmentCount = static_cast<uint32_t>(CreateDesc.m_ColorInputFormatArray.size());
		pipelineRenderingCI.pColorAttachmentFormats = CreateDesc.m_ColorInputFormatArray.data();
		pipelineRenderingCI.depthAttachmentFormat = CreateDesc.m_DepthStencilFormat;
		pipelineRenderingCI.stencilAttachmentFormat = CreateDesc.m_DepthStencilFormat;

		VulkanZeroStruct(VkGraphicsPipelineCreateInfo, graphicPipelineCI);
		graphicPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicPipelineCI.layout = pGraphicPSO->m_Layout;
		graphicPipelineCI.stageCount = static_cast<uint32_t>(shaderPipelineInfos.size());
		graphicPipelineCI.pStages = shaderPipelineInfos.data();
		graphicPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		graphicPipelineCI.pVertexInputState = &vertexInputStateCI;
		graphicPipelineCI.pRasterizationState = &rasterizationStateCI;
		graphicPipelineCI.pColorBlendState = &blendStateCI;
		graphicPipelineCI.pMultisampleState = &multisampleStateCI;
		graphicPipelineCI.pViewportState = &viewportStateCI;
		graphicPipelineCI.pDepthStencilState = &depthStencilStateCI;
		graphicPipelineCI.pDynamicState = &dynamicStateCI;
		graphicPipelineCI.pNext = &pipelineRenderingCI;

		VulkanCheckSucceed(vkCreateGraphicsPipelines(pGraphicPSO->GetRenderDevice().GetNativeDevice(), nullptr, 1, &graphicPipelineCI, nullptr, &pGraphicPSO->m_Pipeline));
		
		for (auto& pShader : pGraphicPSO->m_CreateDesc.m_Shaders)
		{
			pShader->ReleaseGPUShaderObject();
		}

		return pGraphicPSO;
	}

	GraphicPipelineState::~GraphicPipelineState()
	{
		vkDestroyPipeline(GetRenderDevice().GetNativeDevice(), m_Pipeline, nullptr);
	}
}
