#include "RenderGraph.h"

#include "Core/Assertion.h"

#include <queue>
#include <vector>
#include <set>
#include <ranges>
#include <algorithm>
#include <format>

namespace ZE::Render
{
    GraphResourceHandle GraphNode::Read(const GraphResource& resource, RenderBackend::RenderResourceState access)
    {
        ZE_CHECK(m_pRenderGraph);

        GraphResourceHandle handle = m_pRenderGraph->AllocateResource(true);
        handle.m_pOwnerGraphNode = this;
		m_InputResources.push_back(handle);

		return handle;
    }

    GraphResourceHandle GraphNode::Read(GraphResource&& resource, RenderBackend::RenderResourceState access)
    {
		ZE_CHECK(m_pRenderGraph);

		GraphResourceHandle handle = m_pRenderGraph->AllocateResource(true);
		handle.m_pOwnerGraphNode = this;
		m_InputResources.push_back(handle);

		return handle;
    }

    void GraphNode::Read(const GraphResourceHandle& handle, RenderBackend::RenderResourceState access)
    {
		ZE_CHECK(m_pRenderGraph);
        // TODO: move this check to compile-time
        ZE_CHECK(!handle.IsInputResource());

		m_InputResources.push_back(handle);

		handle.m_pOwnerGraphNode->m_SucceedNodes.emplace_back(this);
        m_PrecedeNodes.push_back(handle.m_pOwnerGraphNode);
    }

    GraphResourceHandle GraphNode::Write(GraphResource& resource, RenderBackend::RenderResourceState access)
    {
		ZE_CHECK(m_pRenderGraph);

		GraphResourceHandle handle = m_pRenderGraph->AllocateResource(false);
        m_OutputResources.push_back(handle);

        return handle;
    }

    GraphNode& GraphNode::BindColor(const GraphResource& resource)
    {
        ZE_CHECK(m_ColorAttachments.size() <= kMaxColorAttachments);
        ZE_CHECK(resource.IsTypeOf<GraphResourceType::Texture>());

        auto handle = Read(resource, RenderBackend::RenderResourceState::ColorAttachmentRead);
        m_ColorAttachments.push_back(handle);

        return *this;
    }

    GraphNode& GraphNode::BindDepthStencil(const GraphResource& resource)
    {
		ZE_CHECK(resource.IsTypeOf<GraphResourceType::Texture>());

		auto handle = Read(resource, RenderBackend::RenderResourceState::DepthStencilAttachmentRead);
        m_DepthStencilAttachment = handle;

		return *this;
    }

    GraphNode& GraphNode::BindVertexShader(const std::shared_ptr<RenderBackend::VertexShader>& pVertexShader)
    {
        m_pVertexShader = pVertexShader;
		return *this;
    }

    GraphNode& GraphNode::BindPixelShader(const std::shared_ptr<RenderBackend::PixelShader>& pPixelShader)
    {
		m_pPixelShader = pPixelShader;
		return *this;
    }

    void GraphNode::Execute(NodeJobType&& Job)
    {
        m_Job = std::move(Job);
    }

  //  GraphResourceHandle GraphNode::Write(const GraphResourceHandle& handle)
  //  {
		//ZE_CHECK(handle.m_pGraphNode);
		//// TODO: move this check to compile-time
		//ZE_CHECK(!handle.IsInputResource());

		//GraphResourceHandle newHandle;

		//uint32_t resourceId = m_InputResourceArray.size();
		//m_InputResourceArray.emplace_back(handle.GetResource());

		//newHandle.m_ResourceId = static_cast<int32_t>(resourceId);
		//newHandle.m_ResourceId |= GraphResourceHandle::IsInputNodeMask;
		//newHandle.m_pGraphNode = this;

		//handle.m_pGraphNode->m_Succeeds.emplace_back(this);

		//return newHandle;
  //  }

    //-------------------------------------------------------------------------

    RenderGraphNodeMemoryAllocator::~RenderGraphNodeMemoryAllocator()
    {
        ZE_CHECK(m_Chunks.empty());
    }

    void RenderGraphNodeMemoryAllocator::Release()
    {
        for (auto* pChunk : m_Chunks)
        {
            delete pChunk;
        }
        m_Chunks.clear();
    }

    //-------------------------------------------------------------------------

    RenderGraph::RenderGraph(RenderBackend::RenderDevice& renderDevice)
		: m_RenderDevice(renderDevice), GraphNode(this, "RootNode")
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

        GraphNode* prevTailNode = m_pTailNode;
        m_pTailNode = &result.first->second;

        prevTailNode->m_SucceedNodes.push_back(m_pTailNode);
        m_pTailNode->m_PrecedeNodes.push_back(prevTailNode);

        return *m_pTailNode;
    }

    void RenderGraph::Execute()
    {
        Build();

		for (auto* pNode : m_ExecutionNodes)
		{


			if (pNode->m_Job)
			{
				pNode->m_Job();
			}
		}

		if (!m_ExecutionNodes.empty())
		{
			uint32_t nodeCount = 0;

			std::string nodeSequenceStr;
			for (auto* pNode : m_ExecutionNodes)
			{
				if (++nodeCount < m_ExecutionNodes.size())
				{
					nodeSequenceStr += std::format("{} -> ", pNode->m_NodeName);
				}
				else
				{
					nodeSequenceStr += std::format("{}", pNode->m_NodeName);
				}
			}

			ZE_LOG_INFO("Render graph nodes execution order: {}", nodeSequenceStr);
		}
    }

    void RenderGraph::Build()
    {
        // render graph is the root node
        TopologySort(this);
    }

    void RenderGraph::TopologySort(GraphNode* pGraphNode)
    {
        m_ExecutionNodes.clear();

		using GraphNodeCheckedNodesPair = std::pair<GraphNode*, std::set<GraphNode*>>;
        
		std::vector<GraphNodeCheckedNodesPair> localGraphNodes;
		localGraphNodes.resize(m_GraphNodes.size());

		uint32_t nodeCount = 0;
		for (auto& [_, node] : m_GraphNodes)
		{
            localGraphNodes[nodeCount++] = { &node, { node.m_PrecedeNodes.begin(), node.m_PrecedeNodes.end() } };
		}

        // setup recursive basis

        std::queue<GraphNode*> inspectingNodes;

		auto MarkAsVisited = [&] (GraphNode* pNode) -> bool
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

            if (bHasInspectingNode || pNode == m_pTailNode)
            {
                return true;
            }

			ZE_LOG_ERROR("Render graph has ring! [...->{}->...->{}]", pNode->m_NodeName, pNode->m_NodeName);
            return false;
		};

        ZE_CHECK(MarkAsVisited(this));

        // topology sort

        while (!inspectingNodes.empty())
        {
            auto currNode = inspectingNodes.back();
            inspectingNodes.pop();

            if (!MarkAsVisited(currNode))
            {
                m_ExecutionNodes.clear();
                break;
            }

			m_ExecutionNodes.push_back(currNode);
        }
    }

    GraphResourceHandle RenderGraph::AllocateResource(bool bIsInput)
    {
		GraphResourceHandle handle;

		uint32_t resourceId = m_Resources.size();

        handle.m_ResourceId = static_cast<int32_t>(resourceId);
        if (bIsInput)
        {
            handle.m_ResourceId |= GraphResourceHandle::IsInputNodeMask;
        }

		return handle;
    }

    GraphResource RenderGraph::GetResource(const GraphResourceHandle& handle) const
    {
        ZE_CHECK(handle.m_ResourceId < m_Resources.size());
        ZE_CHECK(handle.m_ResourceId != GraphResourceHandle::InvalidGraphResourceId);
        return m_Resources[handle.m_ResourceId];
    }
}
