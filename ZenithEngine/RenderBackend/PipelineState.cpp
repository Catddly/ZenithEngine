#include "PipelineState.h"

#include "Log/Log.h"
#include "RenderDevice.h"
#include "Shader.h"
#include "VulkanHelper.h"

#include <refl.hpp>

#include <unordered_map>

namespace ZE::RenderBackend
{
	static VkDescriptorType ToVkDescriptorType(Shader::EBindingResourceType type)
	{
		switch (type)
		{
		case ZE::RenderBackend::Shader::EBindingResourceType::Unknown: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		case ZE::RenderBackend::Shader::EBindingResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ZE::RenderBackend::Shader::EBindingResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case ZE::RenderBackend::Shader::EBindingResourceType::Texture2D: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		default:
			break;
		}

		ZE_CHECK(false);
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	static VkShaderStageFlagBits GetShaderVkType(const std::shared_ptr<Shader>& pShader)
	{
		if (pShader->CanDowncastTo<VertexShader>())
		{
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
		else if (pShader->CanDowncastTo<PixelShader>())
		{
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	PipelineState::PipelineState(RenderDevice& renderDevice)
		: m_RenderDevice(renderDevice)
	{}

	bool PipelineState::CreateInPlace(PipelineState* pPipelineState)
	{
		std::unordered_map<Shader::EBindingResourceType, uint32_t> resourceCountMap;
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> setLayoutBindingArray;

		for (auto& pShader : pPipelineState->m_pShaderArray)
		{
			if (!pShader)
			{
				continue;
			}

			const auto& shaderSetArray = pShader->m_Layout.m_ResourceSetArray;
			pPipelineState->m_DescriptorSetLayoutArray.resize(shaderSetArray.size());
			setLayoutBindingArray.resize(shaderSetArray.size());

			uint32_t set = 0;
			for (auto& descriptorSetLayout : pPipelineState->m_DescriptorSetLayoutArray)
			{
				const uint32_t prevBindingCount = setLayoutBindingArray[set].size();
				setLayoutBindingArray[set].resize(shaderSetArray[set].size());

				for (uint32_t i = 0; i < prevBindingCount; ++i)
				{
					setLayoutBindingArray[set][i].stageFlags |= GetShaderVkType(pShader);
				}

				// assume shader layout is consistent across multiple pipeline shaders
				for (uint32_t i = prevBindingCount; i < setLayoutBindingArray.size(); ++i)
				{
					setLayoutBindingArray[set][i].binding = i;
					setLayoutBindingArray[set][i].descriptorCount = 1;
					setLayoutBindingArray[set][i].pImmutableSamplers = nullptr;
					setLayoutBindingArray[set][i].stageFlags = GetShaderVkType(pShader);
					setLayoutBindingArray[set][i].descriptorType = ToVkDescriptorType(shaderSetArray[set][i].m_ResourceType);

					resourceCountMap[shaderSetArray[set][i].m_ResourceType] += 1;
				}

				++set;
			}
		}

		for (uint32_t i = 0; i < setLayoutBindingArray.size(); ++i)
		{
			VulkanZeroStruct(VkDescriptorSetLayoutCreateInfo, setLayoutCI);
			setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindingArray[i].size());
			setLayoutCI.pBindings = setLayoutBindingArray[i].data();
			setLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VulkanCheckSucceed(vkCreateDescriptorSetLayout(pPipelineState->m_RenderDevice.m_Device, &setLayoutCI, nullptr, &pPipelineState->m_DescriptorSetLayoutArray[i]));
		}

		std::vector<VkDescriptorPoolSize> poolSizeArray;
		poolSizeArray.reserve(resourceCountMap.size());
		for (const auto& [type, count] : resourceCountMap)
		{
			poolSizeArray.emplace_back(ToVkDescriptorType(type), count);
		}

		VulkanZeroStruct(VkDescriptorPoolCreateInfo, poolCreateInfo);
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = static_cast<uint32_t>(pPipelineState->m_DescriptorSetLayoutArray.size());
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizeArray.size());
		poolCreateInfo.pPoolSizes = poolSizeArray.data();
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VulkanCheckSucceed(vkCreateDescriptorPool(pPipelineState->m_RenderDevice.m_Device, &poolCreateInfo, nullptr, &pPipelineState->m_DescriptorPool));

		VulkanZeroStruct(VkPipelineLayoutCreateInfo, pipelineLayoutCI);
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(pPipelineState->m_DescriptorSetLayoutArray.size());
		pipelineLayoutCI.pSetLayouts = pPipelineState->m_DescriptorSetLayoutArray.data();

		VulkanCheckSucceed(vkCreatePipelineLayout(pPipelineState->m_RenderDevice.m_Device, &pipelineLayoutCI, nullptr, &pPipelineState->m_Layout));

		return true;
	}

	PipelineState::~PipelineState()
	{
		vkDestroyPipelineLayout(m_RenderDevice.m_Device, m_Layout, nullptr);
	}

	GraphicPipelineState* GraphicPipelineState::Builder::Build(RenderDevice& renderDevice)
	{
		if (!m_pVertexShader || !m_pVertexShader->m_Shader)
		{
			ZE_LOG_ERROR("Graphic pipeline must be created with a valid vertex shader!");
			return nullptr;
		}

		GraphicPipelineState::Settings settings{
			.m_ColorInputFormatArray = std::move(m_ColorInputFormatArray),
			.m_DepthStencilFormat = m_DepthStencilFormat,
		};

		return GraphicPipelineState::Create(renderDevice, settings, m_pVertexShader, m_pPixelShader);
	}

	GraphicPipelineState::GraphicPipelineState(RenderDevice& renderDevice)
		: PipelineState(renderDevice)
	{}

	GraphicPipelineState* GraphicPipelineState::Create(RenderDevice& renderDevice, const Settings& settings, const std::shared_ptr<VertexShader>& pVertexShader, const std::shared_ptr<PixelShader>& pPixelShader)
	{
		GraphicPipelineState* pGraphicPSO = new GraphicPipelineState(renderDevice);

		pGraphicPSO->m_pShaderArray.push_back(pVertexShader);
		pGraphicPSO->m_pShaderArray.push_back(pPixelShader);

		if (!CreateInPlace(pGraphicPSO))
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
		rasterizationStateCI.rasterizerDiscardEnable = VK_TRUE;
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

		memset(&shaderPipelineInfos[0], 0, sizeof(VkPipelineShaderStageCreateInfo));
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
		VulkanZeroStruct(VkPipelineRenderingCreateInfoKHR, pipelineRenderingCI)
		pipelineRenderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingCI.colorAttachmentCount = static_cast<uint32_t>(settings.m_ColorInputFormatArray.size());
		pipelineRenderingCI.pColorAttachmentFormats = settings.m_ColorInputFormatArray.data();
		pipelineRenderingCI.depthAttachmentFormat = settings.m_DepthStencilFormat;
		pipelineRenderingCI.stencilAttachmentFormat = settings.m_DepthStencilFormat;

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

		VulkanCheckSucceed(vkCreateGraphicsPipelines(pGraphicPSO->m_RenderDevice.m_Device, nullptr, 1, &graphicPipelineCI, nullptr, &pGraphicPSO->m_Pipeline));

		pVertexShader->ReleaseGPUShaderObject();
		pPixelShader->ReleaseGPUShaderObject();

		return pGraphicPSO;
	}

	GraphicPipelineState::~GraphicPipelineState()
	{
		vkDestroyPipeline(m_RenderDevice.m_Device, m_Pipeline, nullptr);
	}
}
