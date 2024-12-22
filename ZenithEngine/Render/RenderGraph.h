#pragma once

#include "RenderGraphResource.h"
#include "Core/Assertion.h"
#include "Core/Reflection.h"
#include "Math/Math.h"

#include <cstdint>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>
#include <concepts>
#include <functional>

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

		using NodeJobType = std::function<void()>;

		GraphNode() = default;
		GraphNode(RenderGraph* pRenderGraph, const std::string& nodeName = "Unknown")
			: m_pRenderGraph(pRenderGraph), m_NodeName(nodeName)
		{}

		GraphResourceHandle Read(const GraphResource& resource);
		GraphResourceHandle Read(GraphResource&& resource);
		GraphResourceHandle Read(const GraphResourceHandle& handle);

		GraphResourceHandle Write(GraphResource&& resource);
		//GraphResourceHandle Write(const GraphResourceHandle& handle);

		void Execute(NodeJobType&& Job);

	private:

		std::string								m_NodeName;

		std::vector<GraphResource>				m_InputResourceArray;
		std::vector<GraphResource>				m_OutputResourceArray;

		std::vector<GraphNode*>					m_Precedes;
		std::vector<GraphNode*>					m_Succeeds;
		
		NodeJobType								m_Job;

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

	class RenderGraphNodeMemoryAllocator
	{
		// TODO: align to cache line size to avoid false sharing for multi-threading
		constexpr static uint32_t ChunkMemoryAlignment = 8u;
		constexpr static uint32_t ChunkMemorySizeInByte = 4u * 1024u;

		static_assert(ChunkMemorySizeInByte% ChunkMemoryAlignment == 0);

		struct Chunk
		{
			unsigned char			m_Pad[ChunkMemorySizeInByte];
		};

	public:

		~RenderGraphNodeMemoryAllocator();

		template <typename T>
		T* Allocate();

		/* Release all memories allocated from allocator.
		*  Should only be called when all memories become untouchable by user.
		*/
		void Release();

	private:

		// TODO: single type memory release is not needed now
		//std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>	m_TypeMemoryLocation;

		std::vector<Chunk*>												m_Chunks;
		std::vector<uint32_t>											m_ChunkLeftOverMemorySizeInByte;
	};

	template <typename T>
	T* RenderGraphNodeMemoryAllocator::Allocate()
	{
		// TODO: large memory allocation support
		static_assert(sizeof(T) <= ChunkMemorySizeInByte);

		const uint32_t AllocateSize = sizeof(T);
		const uint32_t AllocateSizeAligned = Math::AlignTo(AllocateSize, ChunkMemoryAlignment);

		//const std::string typeName = GetTypeName<T>();

		//auto iter = m_TypeMemoryLocation.find(typeName);
		//ZE_CHECK(iter == m_TypeMemoryLocation.end());

		uint32_t chunkIndex = 0;
		for (auto& LeftOverMemory : m_ChunkLeftOverMemorySizeInByte)
		{
			// Have enough memory to allocate within this chunk
			if (LeftOverMemory >= AllocateSizeAligned)
			{
				const uint32_t ChunkMeoryAddressOffset = ChunkMemorySizeInByte - LeftOverMemory;
				//m_TypeMemoryLocation.insert({ typeName, { chunkIndex, ChunkMeoryAddressOffset } });

				Chunk* pChunk = m_Chunks[chunkIndex];
				T* pAllocatedMemory = new (reinterpret_cast<unsigned char*>(pChunk) + ChunkMeoryAddressOffset) T();

				LeftOverMemory -= AllocateSizeAligned;
				return pAllocatedMemory;
			
			}

			++chunkIndex;
		}

		Chunk* pChunk = new Chunk;
		m_Chunks.push_back(pChunk);
		auto& LeftOverMemory = m_ChunkLeftOverMemorySizeInByte.emplace_back(ChunkMemorySizeInByte);

		LeftOverMemory -= AllocateSizeAligned;

		//m_TypeMemoryLocation.insert({ typeName, { static_cast<uint32_t>(m_Chunks.size() - 1), 0 }});

		T* pAllocatedMemory = new (pChunk) T();
		return pAllocatedMemory;
	}

	template <typename T>
	concept IsValidNodeResourceType = Core::IsDefaultConstrctible<T> && Core::IsTriviallyDestructible<T>;

	class RenderGraph : public GraphNode
	{
	public:

		RenderGraph();
		~RenderGraph();

		/* Allocate node resource which can be passed into node lambdas.
		*  Node resource type should not have any user-declared destructor and it should have a valid default constructor.
		*  Render graph will take care of its lifetime and destruct all node resources when render graph gets deleted.
		*/
		template <IsValidNodeResourceType T>
		T& AllocateNodeResource();

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
	
		std::unordered_map<std::string, void*>				m_AllocatedMemory;
		RenderGraphNodeMemoryAllocator						m_Allocator;
	};

	template <IsValidNodeResourceType T>
	T& RenderGraph::AllocateNodeResource()
	{
		const std::string typeName = Core::GetTypeName<T>();
		auto iter = m_AllocatedMemory.find(typeName);

		ZE_CHECK(iter == m_AllocatedMemory.end());

		T* pMemory = m_Allocator.Allocate<T>();

		m_AllocatedMemory.insert({ std::move(typeName), pMemory });

		return *pMemory;
	}
}
