#pragma once

#include "RenderGraphResource.h"
#include "Core/Assertion.h"
#include "Core/Reflection.h"
#include "Math/Math.h"
#include "RenderBackend/RenderResourceState.h"

#include <cstdint>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>
#include <concepts>
#include <memory>
#include <functional>
#include <optional>
#include <array>

namespace ZE::RenderBackend { class RenderDevice; class VertexShader; class PixelShader; }

namespace ZE::Render
{
	class GraphResourceHandle
	{
		friend class RenderGraph;
		friend class GraphNode;

	public:

		static constexpr int32_t InvalidGraphResourceId = -1;

	private:

		static constexpr uint32_t IsInputNodeMask = 0x40000000;

		inline bool IsInputResource() const { return (m_ResourceId & IsInputNodeMask) != 0; }

	private:

		int32_t									m_ResourceId = InvalidGraphResourceId;
		GraphNode*								m_pOwnerGraphNode = nullptr;
	};

	class RenderGraph;

	class GraphNode
	{
		friend class RenderGraph;
		friend class GraphResourceHandle;

		constexpr static uint32_t kMaxColorAttachments = 8;

	public:

		using NodeJobType = std::function<void()>;

		GraphNode() = default;
		GraphNode(RenderGraph* pRenderGraph, const std::string& nodeName = "Unknown")
			: m_pRenderGraph(pRenderGraph), m_NodeName(nodeName)
		{}

		GraphResourceHandle Read(const GraphResource& resource, RenderBackend::RenderResourceState access);
		GraphResourceHandle Read(GraphResource&& resource, RenderBackend::RenderResourceState access);
		void Read(const GraphResourceHandle& handle, RenderBackend::RenderResourceState access);

		GraphResourceHandle Write(GraphResource& resource, RenderBackend::RenderResourceState access);
		//GraphResourceHandle Write(const GraphResourceHandle& handle);

		GraphNode& BindColor(const GraphResource& resource);
		GraphNode& BindDepthStencil(const GraphResource& resource);

		GraphNode& BindVertexShader(const std::shared_ptr<RenderBackend::VertexShader>& pVertexShader);
		GraphNode& BindPixelShader(const std::shared_ptr<RenderBackend::PixelShader>& pPixelShader);

		void Execute(NodeJobType&& Job);

	private:

		std::string								m_NodeName;

		std::vector<GraphResourceHandle>		m_InputResources;
		std::vector<GraphResourceHandle>		m_OutputResources;

		// TODO: separate graphic pipeline payload from GraphNode
		std::vector<GraphResourceHandle>					m_ColorAttachments = {};
		std::optional<GraphResourceHandle>					m_DepthStencilAttachment;
		std::shared_ptr<RenderBackend::VertexShader>		m_pVertexShader = nullptr;
		std::shared_ptr<RenderBackend::PixelShader>			m_pPixelShader = nullptr;

		std::vector<GraphNode*>					m_PrecedeNodes;
		std::vector<GraphNode*>					m_SucceedNodes;
		
		NodeJobType								m_Job;

		RenderGraph*							m_pRenderGraph = nullptr;
	};

	class RenderGraphNodeMemoryAllocator
	{
		// TODO: align to cache line size to avoid false sharing for multi-threading
		constexpr static uint32_t ChunkMemoryAlignment = 8u;
		constexpr static uint32_t ChunkMemorySizeInByte = 4u * 1024u;

		static_assert(ChunkMemorySizeInByte % ChunkMemoryAlignment == 0);

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
				T* pAllocatedMemory = new (reinterpret_cast<volatile unsigned char*>(pChunk) + ChunkMeoryAddressOffset) T();

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
		friend class GraphNode;
		friend class GraphResourceHandle;

	public:

		template <GraphResourceType Type>
		using GraphResourceUnderlyingType = typename GraphResourceTrait<Type>::ResourceType;

		template <GraphResourceType Type>
		using GraphResourceStorageType = typename GraphResourceTrait<Type>::ResourceStorageType;

		template <GraphResourceType Type>
		using GraphResourceDescType = typename GraphResourceTrait<Type>::ResourceDescType;

		RenderGraph(RenderBackend::RenderDevice& renderDevice);
		~RenderGraph();

		template <ValidUnderlyingGraphResource T, typename... Args>
		[[nodiscard("Allocated graph resource must be used.")]] inline GraphResource CreateResource(const T& desc, Args... args);

		template <typename T>
		[[nodiscard("Imported graph resource must be used.")]] inline GraphResource ImportResource(const std::shared_ptr<T>& resource);

		/* Allocate node resource which can be passed into node lambdas.
		*  Node resource type should not have any user-declared destructor and it should have a valid default constructor.
		*  Render graph will take care of its lifetime and destruct all node resources when render graph gets deleted.
		*/
		template <IsValidNodeResourceType T>
		T& AllocateNodeResource();

		[[nodiscard("Allocated graph node must be used.")]] GraphNode& AddNode(const std::string& nodeName);

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

		GraphResourceHandle AllocateResource(bool bIsInput);
		GraphResource GetResource(const GraphResourceHandle& handle) const;

	private:

		RenderBackend::RenderDevice&						m_RenderDevice;

		GraphNode*											m_pTailNode = this;
		std::unordered_map<std::string, GraphNode>			m_GraphNodes;

		std::vector<GraphNode*>								m_ExecutionNodes;
	
		std::unordered_map<std::string, void*>				m_AllocatedMemory;
		RenderGraphNodeMemoryAllocator						m_Allocator;

		std::vector<GraphResource>							m_Resources;
	};

	template <ValidUnderlyingGraphResource T, typename... Args>
	inline GraphResource RenderGraph::CreateResource(const T& desc, Args... args)
	{
		constexpr GraphResourceType resourceType = GraphUnderlyingResourceTarit<T>::type;
		auto* pResource = GraphResourceUnderlyingType<resourceType>::Create(m_RenderDevice, desc, std::forward<Args>(args)...);
		return std::shared_ptr<GraphResourceUnderlyingType<resourceType>>(pResource);
	}

	template<typename T>
	inline GraphResource RenderGraph::RenderGraph::ImportResource(const std::shared_ptr<T>& resource)
	{
		static_assert(ValidUnderlyingGraphResource<T>);
		return GraphResource(resource);
	}

	template <IsValidNodeResourceType T>
	T& RenderGraph::AllocateNodeResource()
	{
		const std::string typeName = Core::GetTypeName_Direct<T>();
		auto iter = m_AllocatedMemory.find(typeName);

		ZE_CHECK(iter == m_AllocatedMemory.end());

		T* pMemory = m_Allocator.Allocate<T>();

		m_AllocatedMemory.insert({ std::move(typeName), pMemory });

		return *pMemory;
	}
}
