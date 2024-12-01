#pragma once

#include "RenderGraphResource.h"

#include <cstdint>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>
//#include <concepts>

namespace ZE::RenderGraph
{
	//static constexpr uint32_t MaxGraphNodeResourceCount = 10u;

	//template <uint32_t InputCount, uint32_t OutputCount>
	//class GraphNodeInterface
	//{
	//	//friend class RenderGraph;

	//public:

	//private:

	//	std::array<GraphResource, InputCount>		m_InputResourceArray;
	//	std::array<GraphResource, OutputCount>		m_OutputResourceArray;
	//};

	class GraphResourceHandle
	{
		friend class GraphNode;

	public:

		static constexpr int32_t InvalidGraphResourceId = -1;

	private:

		static constexpr uint32_t IsInputNodeMask = 0x40000000;

		inline bool IsInputResource() const { return (m_ResourceId & IsInputNodeMask) != 0; }
		GraphResource GetResource() const;

	private:

		int32_t									m_ResourceId = InvalidGraphResourceId;
		GraphNode*								m_pGraphNode = nullptr;
	};

	class RenderGraph;

	class GraphNode
	{
		friend class RenderGraph;
		friend class GraphResourceHandle;

	public:

		GraphNode() = default;
		GraphNode(RenderGraph* pRenderGraph, const std::string& nodeName = "Unknown")
			: m_pRenderGraph(pRenderGraph), m_NodeName(nodeName)
		{}

		GraphResourceHandle Read(const GraphResource& resource);
		GraphResourceHandle Read(GraphResource&& resource);
		GraphResourceHandle Read(const GraphResourceHandle& handle);

		GraphResourceHandle Write(GraphResource&& resource);
		//GraphResourceHandle Write(const GraphResourceHandle& handle);

	private:

		std::string								m_NodeName;

		std::vector<GraphResource>				m_InputResourceArray;
		std::vector<GraphResource>				m_OutputResourceArray;

		std::vector<GraphNode*>					m_Precedes;
		std::vector<GraphNode*>					m_Succeeds;
		
		RenderGraph*							m_pRenderGraph = nullptr;
	};

	//template <
	//	uint32_t InputCount0, uint32_t OutputCount0, template<uint32_t, uint32_t> typename GN0,
	//	uint32_t InputCount1, uint32_t OutputCount1, template<uint32_t, uint32_t> typename GN1
	//>
	//concept InputOutputGraphNode = requires
	//{
	//	requires std::same_as<GN0<InputCount0, OutputCount0>, GraphNode<InputCount0, OutputCount0>>;
	//	requires std::same_as<GN1<InputCount1, OutputCount1>, GraphNode<InputCount1, OutputCount1>>;
	//	requires OutputCount0 == InputCount1;
	//};

	//template <uint32_t ICIn, uint32_t OCIn, uint32_t ICOut, uint32_t OCOut>
	//concept ConsequentGraphNodePair = requires
	//{
	//	requires OCIn == ICOut;
	//};

	// Dev Test Only
	//template <
	//	uint32_t ICIn, uint32_t OCIn, template<uint32_t, uint32_t> typename GNIn,
	//	uint32_t ICOut, uint32_t OCOut, template<uint32_t, uint32_t> typename GNOut
	//>
	//struct GraphNodeFlow
	//{
	//	GNIn<ICIn, OCIn>*						m_NodeIn;
	//	GNOut<ICOut, OCOut>*					m_NodeOut;
	//};

	//template <uint32_t ICIn, uint32_t OCIn, uint32_t ICOut, uint32_t OCOut>
	//struct GraphNodeFlow
	//{
	//	static_assert(OCIn == ICOut, "Outputs of input node should match the inputs of output node.b");

	//	GraphNode<ICIn, OCIn>*					m_NodeIn;
	//	GraphNode<ICOut, OCOut>*				m_NodeOut;
	//};

	//class GraphNodeBuilder
	//{

	//};

	class RenderGraph : public GraphNode
	{
	public:

		RenderGraph();

		GraphNode* AddNode(const std::string& nodeName);

		/* Execute render graph.
		*  All graph nodes will be executed.
		*  Allocated dedicated memory owned by graph node will be release after execution.
		*/
		void Execute();

	private:

		/* Build render graph by node dependencies.
		*  Sequential execution nodes will be produced.
		*  Then this render graph can be executed.
		*/
		void Build();
		void TopologySort(GraphNode* pGraphNode);

	private:

		GraphNode*											m_pTailNode = this;
		std::unordered_map<std::string, GraphNode>			m_GraphNodes;

		std::vector<GraphNode*>								m_ExecutionNodes;
	};

	//template<uint32_t InputCount, uint32_t OutputCount>
	//void RenderGraph::AddNode(GraphNode<InputCount, OutputCount>&& node)
	//{
	//	m_SequentialExecuteNodeArray.emplace_back(node);
	//}
}
