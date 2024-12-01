#include "RenderGraph.h"

#include "Core/Assertion.h"

#include <queue>
#include <vector>
#include <set>
#include <ranges>
#include <algorithm>
#include <format>

namespace ZE::RenderGraph
{
	GraphResource GraphResourceHandle::GetResource() const
	{
		ZE_CHECK(m_pGraphNode);
        ZE_CHECK(m_ResourceId != InvalidGraphResourceId);

        if (IsInputResource())
        {
            return m_pGraphNode->m_InputResourceArray[m_ResourceId];
        }

        return m_pGraphNode->m_OutputResourceArray[m_ResourceId];
	}

    //-------------------------------------------------------------------------

    GraphResourceHandle GraphNode::Read(const GraphResource& resource)
    {
		GraphResourceHandle handle;

		uint32_t resourceId = m_InputResourceArray.size();
		m_InputResourceArray.emplace_back(resource);

		handle.m_ResourceId = static_cast<int32_t>(resourceId);
		handle.m_ResourceId |= GraphResourceHandle::IsInputNodeMask;
		handle.m_pGraphNode = this;

		return handle;
    }

    GraphResourceHandle GraphNode::Read(GraphResource&& resource)
    {
        GraphResourceHandle handle;
     
        uint32_t resourceId = m_InputResourceArray.size();
        m_InputResourceArray.emplace_back(resource);
        
        handle.m_ResourceId = static_cast<int32_t>(resourceId);
        handle.m_ResourceId |= GraphResourceHandle::IsInputNodeMask;
        handle.m_pGraphNode = this;

        return handle;
    }

    GraphResourceHandle GraphNode::Read(const GraphResourceHandle& handle)
    {
        ZE_CHECK(handle.m_pGraphNode);
        // TODO: move this check to compile-time
        ZE_CHECK(!handle.IsInputResource());

		GraphResourceHandle newHandle;

		uint32_t resourceId = m_InputResourceArray.size();
		m_InputResourceArray.emplace_back(handle.GetResource());

        newHandle.m_ResourceId = static_cast<int32_t>(resourceId);
        newHandle.m_ResourceId |= GraphResourceHandle::IsInputNodeMask;
        newHandle.m_pGraphNode = this;

		handle.m_pGraphNode->m_Succeeds.emplace_back(this);
        m_Precedes.push_back(handle.m_pGraphNode);

		return newHandle;
    }

    GraphResourceHandle GraphNode::Write(GraphResource&& resource)
    {
		GraphResourceHandle handle;

		uint32_t resourceId = m_OutputResourceArray.size();
        m_OutputResourceArray.emplace_back(resource);

		handle.m_ResourceId = static_cast<int32_t>(resourceId);
		handle.m_pGraphNode = this;
		return handle;
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

    RenderGraph::RenderGraph()
		: GraphNode(this, "RootNode")
    {
    }

    GraphNode* RenderGraph::AddNode(const std::string& nodeName)
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

        prevTailNode->m_Succeeds.push_back(m_pTailNode);
        m_pTailNode->m_Precedes.push_back(prevTailNode);

        return m_pTailNode;
    }

    void RenderGraph::Execute()
    {
        Build();
    }

    void RenderGraph::Build()
    {
        // render graph is the root node
        TopologySort(this);

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

    void RenderGraph::TopologySort(GraphNode* pGraphNode)
    {
        m_ExecutionNodes.clear();

		using GraphNodeCheckedNodesPair = std::pair<GraphNode*, std::set<GraphNode*>>;
        
		std::vector<GraphNodeCheckedNodesPair> localGraphNodes;
		localGraphNodes.resize(m_GraphNodes.size());

		uint32_t nodeCount = 0;
		for (auto& [_, node] : m_GraphNodes)
		{
            localGraphNodes[nodeCount++] = { &node, { node.m_Precedes.begin(), node.m_Precedes.end() } };
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
}
