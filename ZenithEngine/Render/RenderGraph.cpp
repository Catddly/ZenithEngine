#include "RenderGraph.h"

#include "Core/Assertion.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/PipelineState.h"
#include "RenderBackend/RenderCommandList.h"
#include "RenderBackend/DescriptorCache.h"
#include "RenderBackend/PipelineStateCache.h"
#include "RenderBackend/VulkanHelper.h"
#include "RenderBackend/RenderWindow.h"

#include <vulkan/vulkan_core.h>

#include <queue>
#include <vector>
#include <set>
#include <ranges>
#include <algorithm>
#include <forward_list>

namespace ZE::Render
{
	namespace
	{
		std::vector<RenderBackend::BufferBarrier> gsTempBufferBarriers;
		std::vector<RenderBackend::TextureBarrier> gsTempTextureBarriers;

		std::forward_list<RenderBackend::ERenderResourceState> gsTempPrevResourceTransitionStates;
		std::forward_list<RenderBackend::ERenderResourceState> gsTempNextResourceTransitionStates;
		
		VkDescriptorType ToVkDescriptorType(RenderBackend::EShaderBindingResourceType type)
		{
			switch ( type )
			{
				case RenderBackend::EShaderBindingResourceType::Unknown: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
				case RenderBackend::EShaderBindingResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				case RenderBackend::EShaderBindingResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				case RenderBackend::EShaderBindingResourceType::Texture2D: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			
			ZE_UNREACHABLE();
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}
	
	GraphExecutionContext::GraphExecutionContext(RenderGraph& renderGraph)
		: m_RenderGraph(renderGraph)
	{}
	
	void GraphExecutionContext::SetViewportSize(const glm::uvec2& viewportSize) const
	{
		ZE_CHECK(m_RenderTargetPtrs && m_RenderTargetBindings);
		
		m_RenderCommandList->CmdBeginDynamicRendering(viewportSize, *m_RenderTargetPtrs, *m_RenderTargetBindings);
		m_RenderCommandList->CmdSetViewport(viewportSize);
		m_RenderCommandList->CmdSetScissor(viewportSize);
	}
	
	void GraphExecutionContext::BindVertexInput(const GraphResourceHandle& vertexBufferHandle, const GraphResourceHandle& indexBufferHandle) const
	{
		const auto& vertexBuffer = m_RenderGraph.get().GetResource(vertexBufferHandle);

		if (indexBufferHandle.IsValid())
		{
			const auto& indexBuffer = m_RenderGraph.get().GetResource(indexBufferHandle);
			m_RenderCommandList->CmdBindVertexInput(
				vertexBuffer.GetResourceStorage<GraphResourceType::Buffer>().get(),
				indexBuffer.GetResourceStorage<GraphResourceType::Buffer>().get());
		}
		else
		{
			m_RenderCommandList->CmdBindVertexInput(vertexBuffer.GetResourceStorage<GraphResourceType::Buffer>().get());
		}
	}

	void GraphExecutionContext::BindResource(const std::string& name, const GraphResourceHandle& handle)
	{
		if (m_PipelineState && m_DescriptorSets)
		{
			const auto& location = m_PipelineState->FindBoundResourceLocation(name);

			VulkanZeroStruct(VkWriteDescriptorSet, writeSet);
			writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = ToVkDescriptorType(m_PipelineState->FindBoundResourceType(name));
			writeSet.dstBinding = location.GetBindingIndex();
			writeSet.dstSet = (*m_DescriptorSets)[location.GetSetIndex()];

			auto& resource = m_RenderGraph.get().GetResource(handle);
			if (resource.IsTypeOf<GraphResourceType::Buffer>())
			{
				VkDescriptorBufferInfo bufferInfo;
				bufferInfo.buffer = resource.GetResourceStorage<GraphResourceType::Buffer>()->GetNativeHandle();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;
				m_TemporaryBufferInfos.push_front(bufferInfo);

				writeSet.pBufferInfo = &m_TemporaryBufferInfos.front();
				m_WriteDescriptorSets.resize(location.GetSetIndex() + 1);
				m_WriteDescriptorSets[location.GetSetIndex()].emplace_back(writeSet);
			}
			else if (resource.IsTypeOf<GraphResourceType::Texture>())
			{
				auto& resourceStorage = resource.GetResourceStorage<GraphResourceType::Texture>();
				VkDescriptorImageInfo imageInfo;
				imageInfo.imageView = resourceStorage->GetOrCreateView();
				imageInfo.imageLayout = GetTextureLayout(m_RenderGraph.get().GetResourceState(handle));
				// TODO: static sampler
				imageInfo.sampler = nullptr;
				m_TemporaryImageInfos.push_front(imageInfo);
				
				writeSet.pImageInfo = &m_TemporaryImageInfos.front();
				m_WriteDescriptorSets.resize(location.GetSetIndex() + 1);
				m_WriteDescriptorSets[location.GetSetIndex()].emplace_back(writeSet);
			}
			else
			{
				ZE_CHECK_LOG(false, "Unknown or invalid resource type.");
			}
		}
	}
	
	void GraphExecutionContext::BindPipeline()
	{
		if (m_RenderCommandList)
		{
			for (const auto& set : m_WriteDescriptorSets)
			{
				vkUpdateDescriptorSets(m_RenderGraph.get().m_RenderDevice.get().GetNativeDevice(),
					static_cast<uint32_t>(set.size()), set.data(),
					0, nullptr);
			}
			m_WriteDescriptorSets.clear();
			m_TemporaryBufferInfos.clear();
			m_TemporaryImageInfos.clear();
			
			m_RenderCommandList->CmdBindShaderResource(m_PipelineBindPoint, m_PipelineState, *m_DescriptorSets);
			m_RenderCommandList->CmdBindPipeline(m_PipelineBindPoint, m_PipelineState);
		}
		else
		{
			ZE_LOG_WARNING("Try to execute command in unbounded render graph command list!");
		}
	}
	
	void GraphExecutionContext::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
	{
		m_RenderCommandList->CmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

    void GraphNode::Read(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState access)
    {
		ZE_CHECK(m_RenderGraph);

		m_InputResources.push_back(handle);
		m_InputResourceStates.push_back(access);
    }
	
    void GraphNode::Write(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState access)
	{
		ZE_CHECK(m_RenderGraph);
		
		m_OutputResources.push_back(handle);
		m_OutputResourceStates.push_back(access);
	}

    GraphNode& GraphNode::AddColorRenderTarget(const GraphResourceHandle& handle, RenderBackend::ERenderTargetLoadOperation loadOp, RenderBackend::ERenderTargetStoreOperation storeOp, const glm::vec4& clearColor)
    {
        ZE_CHECK(m_ColorAttachments.size() <= kMaxColorAttachments);
        ZE_CHECK(m_RenderGraph->GetResource(handle).IsTypeOf<GraphResourceType::Texture>());

        m_ColorAttachments.push_back(handle);
		m_ColorAttachmentBindings.emplace_back(loadOp, storeOp, clearColor);
        return *this;
    }
	
    GraphNode& GraphNode::BindDepthStencilRenderTarget(const GraphResourceHandle& handle, RenderBackend::ERenderTargetLoadOperation loadOp, RenderBackend::ERenderTargetStoreOperation storeOp, const RenderBackend::DepthStencilClearValue& clearValue)
    {
		ZE_CHECK(m_RenderGraph->GetResource(handle).IsTypeOf<GraphResourceType::Texture>());

        m_DepthStencilAttachment = handle;
		m_DepthStencilAttachmentBinding = {.m_LoadOp = loadOp, .m_StoreOp = storeOp, .m_ClearValue = clearValue};
		return *this;
    }

    GraphNode& GraphNode::BindVertexShader(std::shared_ptr<RenderBackend::VertexShader> pVertexShader)
    {
        m_VertexShader = std::move(pVertexShader);
		return *this;
    }

    GraphNode& GraphNode::BindPixelShader(std::shared_ptr<RenderBackend::PixelShader> pPixelShader)
    {
		m_PixelShader = std::move(pPixelShader);
		return *this;
    }

    void GraphNode::Execute(NodeJobType&& job)
    {
        m_Job = std::move(job);
    }
	
    //-------------------------------------------------------------------------

    RenderGraphNodeMemoryAllocator::~RenderGraphNodeMemoryAllocator()
    {
        ZE_CHECK(m_Chunks.empty());
    }

    void RenderGraphNodeMemoryAllocator::Release()
    {
		// TODO: reuse by memory pool
        for (auto* pChunk : m_Chunks)
        {
            delete pChunk;
        }
        m_Chunks.clear();
    }

    //-------------------------------------------------------------------------

    RenderGraph::RenderGraph(RenderBackend::RenderDevice& renderDevice)
		: GraphNode(this, "RootNode"), m_RenderDevice(renderDevice)
    {
    }

    RenderGraph::~RenderGraph()
    {
        m_Allocator.Release();
    }

    GraphNode& RenderGraph::AddNode(const std::string& nodeName)
    {
        auto iter = m_GraphNodes.find(nodeName);
        ZE_CHECK(iter == m_GraphNodes.end());

        auto result = m_GraphNodes.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(nodeName),
            std::forward_as_tuple(this, nodeName)
        );
        ZE_CHECK(result.second);

        GraphNode* prevTailNode = m_TailNode;
        m_TailNode = &result.first->second;

        prevTailNode->m_SucceedNodes.push_back(m_TailNode);
        m_TailNode->m_PrecedeNodes.push_back(prevTailNode);

        return *m_TailNode;
    }
	
    void RenderGraph::Execute(RenderBackend::PipelineStateCache& pipelineStateCache, RenderBackend::RenderWindow& renderWindow)
    {
        Build();

		ZE_CHECK_LOG(m_Resources.size() == m_CurrentResourcesStates.size(), "Inconsistent number of graph resources and its states!");
		
		GraphExecutionContext context(*this);
		auto* pFrameCmdList = m_RenderDevice.get().GetFrameCommandList();
		
		pFrameCmdList->BeginRecord();
		for (const auto* pNode : m_ExecutionNodes)
		{
			ZE_CHECK_LOG(pNode->m_InputResources.size() == pNode->m_InputResourceStates.size(), "Inconsistent number of node {} input resources and its states!", pNode->m_NodeName.c_str());
			ZE_CHECK_LOG(pNode->m_OutputResources.size() == pNode->m_OutputResourceStates.size(), "Inconsistent number of node {} output resources and its states!", pNode->m_NodeName.c_str());

			gsTempBufferBarriers.reserve(pNode->m_InputResourceStates.size());
			gsTempTextureBarriers.reserve(pNode->m_InputResourceStates.size());
			
			// barrier transition
			for (uint32_t i = 0; i < pNode->m_InputResourceStates.size(); ++i)
			{
				auto dstResourceState = pNode->m_InputResourceStates[i];
				if (IsResourceStateChanged(pNode->m_InputResources[i], dstResourceState))
				{
					TransitionResource(pNode->m_InputResources[i], dstResourceState, pFrameCmdList->GetQueueIndex());
				}
			}
			for (uint32_t i = 0; i < pNode->m_OutputResourceStates.size(); ++i)
			{
				auto dstResourceState = pNode->m_OutputResourceStates[i];
				if (IsResourceStateChanged(pNode->m_OutputResources[i], dstResourceState))
				{
					TransitionResource(pNode->m_OutputResources[i], dstResourceState, pFrameCmdList->GetQueueIndex());
				}
			}
			pFrameCmdList->CmdResourceBarrier(nullptr, gsTempBufferBarriers, gsTempTextureBarriers);
			
			gsTempBufferBarriers.clear();
			gsTempTextureBarriers.clear();
			gsTempPrevResourceTransitionStates.clear();
			gsTempNextResourceTransitionStates.clear();
			
			if (pNode->m_Job)
			{
				// Create pipeline state here? may be far more ahead
				RenderBackend::GraphicPipelineStateCreateDesc graphicPSOCreateDesc;
				std::vector<RenderBackend::Texture*> renderTargetPtrs;
				std::vector<RenderBackend::RenderPassRenderTargetBinding> renderTargetBindings;

				for (auto i = 0u; i < pNode->m_ColorAttachments.size(); ++i)
				{
					const auto& resource = GetResource(pNode->m_ColorAttachments[i]);
					ZE_CHECK(resource.IsTypeOf<GraphResourceType::Texture>());
					
					graphicPSOCreateDesc.AddColorOutput(resource.GetDesc<GraphResourceType::Texture>().m_Format);

					renderTargetPtrs.push_back(resource.GetResourceStorage<GraphResourceType::Texture>().get());
					renderTargetBindings.push_back(pNode->m_ColorAttachmentBindings[i]);
				}

				if (pNode->m_DepthStencilAttachment.has_value())
				{
					const auto& resource = GetResource(*pNode->m_DepthStencilAttachment);
					ZE_CHECK(resource.IsTypeOf<GraphResourceType::Texture>());
					graphicPSOCreateDesc.SetDepthStencilOutput(resource.GetDesc<GraphResourceType::Texture>().m_Format);

					renderTargetPtrs.push_back(resource.GetResourceStorage<GraphResourceType::Texture>().get());
					renderTargetBindings.push_back(pNode->m_DepthStencilAttachmentBinding);
				}

				graphicPSOCreateDesc.SetVertexShader(pNode->m_VertexShader);
				graphicPSOCreateDesc.SetPixelShaderOptional(pNode->m_PixelShader);

				auto pPipelineState = pipelineStateCache.CreateGraphicPipelineState(graphicPSOCreateDesc);
				if (pPipelineState)
				{
					auto& sets = m_RenderDevice.get().GetFrameDescriptorCache()->FindOrAdd(pPipelineState.get());
				
					context.SetDescriptorSets(sets);
					context.SetPipeline(pPipelineState.get(), VK_PIPELINE_BIND_POINT_GRAPHICS);
					context.SetRenderTargets(&renderTargetPtrs, &renderTargetBindings);
					context.SetCommandList(*pFrameCmdList);
				
					pNode->m_Job(context);

					pFrameCmdList->CmdEndDynamicRendering();	
				}
			}
		}
		pFrameCmdList->EndRecord();

		m_RenderDevice.get().SubmitCommandList(pFrameCmdList, renderWindow);
    }

    void RenderGraph::Build()
    {
        // render graph is the root node
        TopologySort();
    }

    void RenderGraph::TopologySort()
    {
        m_ExecutionNodes.clear();

		using GraphNodeCheckedNodesPair = std::pair<GraphNode*, std::set<GraphNode*>>;
        
		std::vector<GraphNodeCheckedNodesPair> localGraphNodes;
		localGraphNodes.resize(m_GraphNodes.size());

		uint32_t nodeCount = 0;
		for (auto& node : std::views::values(m_GraphNodes))
		{
            localGraphNodes[nodeCount++] = { &node, { node.m_PrecedeNodes.begin(), node.m_PrecedeNodes.end() } };
		}

        // setup recursive basis
        std::queue<GraphNode*> inspectingNodes;

		auto fMarkAsVisited = [&] (GraphNode* pNode) -> bool
		{
            if (!pNode)
            {
				ZE_LOG_ERROR("Render graph is visiting an empty node!");
				return false;
            }

            bool bHasInspectingNode = false;

			for (auto& pair : localGraphNodes)
			{
				if (auto iter = pair.second.find(pNode); iter != pair.second.end())
				{
					pair.second.erase(iter);
                    if (pair.second.empty())
                    {
                        inspectingNodes.push(pair.first);

                        bHasInspectingNode = true;
                    }
				}
			}

            if (bHasInspectingNode || pNode == m_TailNode)
            {
                return true;
            }

			ZE_LOG_ERROR("Render graph has ring! [...->{}->...->{}]", pNode->m_NodeName, pNode->m_NodeName);
            return false;
		};

		ZE_EXEC_CHECK(fMarkAsVisited(this));

        // topology sort
        while (!inspectingNodes.empty())
        {
            auto currNode = inspectingNodes.back();
            inspectingNodes.pop();

            if (!fMarkAsVisited(currNode))
            {
                m_ExecutionNodes.clear();
                break;
            }

			m_ExecutionNodes.push_back(currNode);
        }
    }

    const GraphResource& RenderGraph::GetResource(const GraphResourceHandle& handle) const
    {
    	const uint32_t resourceIndex = GetResourceIndex(handle);
        return m_Resources[resourceIndex];
    }
	
    uint32_t RenderGraph::GetResourceIndex(const GraphResourceHandle& handle) const
	{
		ZE_CHECK(handle.IsValid());
		const auto index = handle.m_ResourceId;
		ZE_CHECK(index < static_cast<uint32_t>(m_Resources.size()));
		return index;
	}

    bool RenderGraph::IsResourceStateChanged(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState state) const
	{
		const auto index = GetResourceIndex(handle);
		return m_CurrentResourcesStates[index] != state;
	}
	
	RenderBackend::ERenderResourceState RenderGraph::GetResourceState(const GraphResourceHandle& handle) const
	{
		const auto index = GetResourceIndex(handle);
		return m_CurrentResourcesStates[index];
	}

	void RenderGraph::UpdateResourceState(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState state)
	{
		const auto index = GetResourceIndex(handle);
		m_CurrentResourcesStates[index] = state;
	}
	
	void RenderGraph::TransitionResource(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState dstResourceState, uint32_t queueIndex)
	{
		auto& resource = GetResource(handle);
		gsTempPrevResourceTransitionStates.push_front(GetResourceState(handle));
		gsTempNextResourceTransitionStates.push_front(dstResourceState);
		
		if (resource.IsTypeOf<GraphResourceType::Buffer>())
		{
			const auto& storage = resource.GetResourceStorage<GraphResourceType::Buffer>();
			RenderBackend::BufferBarrier bufferBarrier;
			bufferBarrier.m_Buffer = storage.get()->GetNativeHandle();
			bufferBarrier.m_Size = storage.get()->GetDesc().m_Size;
			bufferBarrier.m_Offset = 0;
			bufferBarrier.m_PrevAccesses = std::span{&gsTempPrevResourceTransitionStates.front(), 1};
			bufferBarrier.m_NextAccesses = std::span{&gsTempNextResourceTransitionStates.front(), 1};
			bufferBarrier.m_SrcQueueFamilyIndex = queueIndex;
			bufferBarrier.m_DstQueueFamilyIndex = queueIndex;
			gsTempBufferBarriers.push_back(bufferBarrier);
		}
		else if (resource.IsTypeOf<GraphResourceType::Texture>())
		{
			const auto& storage = resource.GetResourceStorage<GraphResourceType::Texture>();
			RenderBackend::TextureBarrier textureBarrier;
			textureBarrier.m_Texture = storage.get()->GetNativeHandle();
			textureBarrier.m_PrevAccesses = std::span{&gsTempPrevResourceTransitionStates.front(), 1};
			textureBarrier.m_NextAccesses = std::span{&gsTempNextResourceTransitionStates.front(), 1};
			textureBarrier.m_SrcQueueFamilyIndex = queueIndex;
			textureBarrier.m_DstQueueFamilyIndex = queueIndex;
			textureBarrier.m_SubresourceRange = RenderBackend::TextureSubresourceRange::AllSubresources(SpeculateVkImageAspectFlagsFromDesc(storage.get()->GetDesc()));
			gsTempTextureBarriers.push_back(textureBarrier);
		}
		else
		{
			ZE_CHECK(false);
		}
		UpdateResourceState(handle, dstResourceState);
	}
}
