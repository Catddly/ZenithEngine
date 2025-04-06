#pragma once

#include "RenderGraphResource.h"
#include "Core/Assertion.h"
#include "Core/Reflection.h"
#include "Math/Math.h"
#include "RenderBackend/RenderResourceState.h"
#include "RenderBackend/RenderCommandList.h"
#include "RenderBackend/RenderPass.h"
#include "RenderBackend/RenderDevice.h"

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <concepts>
#include <memory>
#include <functional>
#include <optional>
#include <forward_list>

namespace ZE::RenderBackend
{
	class PipelineStateCache; class PipelineState;
	class RenderDevice;
	class VertexShader; class PixelShader;
}

namespace ZE::Render
{
	class GraphResourceHandle
	{
		friend class RenderGraph;
		friend class GraphNode;

	public:

		static constexpr uint32_t InvalidGraphResourceId = std::numeric_limits<uint32_t>::max();

		inline bool IsValid() const { return m_ResourceId != InvalidGraphResourceId; }	

	private:

		uint32_t								m_ResourceId = InvalidGraphResourceId;
	};

	class GraphExecutionContext;
	class RenderGraph;
	
	class GraphNode
	{
		friend class RenderGraph;
		friend class GraphResourceHandle;

		constexpr static uint32_t kMaxColorAttachments = 8;

	public:

		using NodeJobType = std::function<void(GraphExecutionContext&)>;

		GraphNode() = default;
		GraphNode(RenderGraph* pRenderGraph, std::string nodeName = "Unknown")
			: m_NodeName(std::move(nodeName)), m_RenderGraph(pRenderGraph)
		{}

		// void Read(const GraphResource& resource, RenderBackend::ERenderResourceState access);
		void Read(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState access);

		// void Write(GraphResource& resource, RenderBackend::ERenderResourceState access);
		void Write(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState access);

		GraphNode& AddColorRenderTarget(const GraphResourceHandle& handle,
			RenderBackend::ERenderTargetLoadOperation loadOp = RenderBackend::ERenderTargetLoadOperation::DontCare,
			RenderBackend::ERenderTargetStoreOperation storeOp = RenderBackend::ERenderTargetStoreOperation::Store,
			const glm::vec4& clearColor = RenderBackend::CommonColors::m_Transparent);
		GraphNode& BindDepthStencilRenderTarget(const GraphResourceHandle& handle,
			RenderBackend::ERenderTargetLoadOperation loadOp = RenderBackend::ERenderTargetLoadOperation::DontCare,
			RenderBackend::ERenderTargetStoreOperation storeOp = RenderBackend::ERenderTargetStoreOperation::Store,
			const RenderBackend::DepthStencilClearValue& clearValue = {});

		GraphNode& BindVertexShader(std::shared_ptr<RenderBackend::VertexShader> pVertexShader);
		GraphNode& BindPixelShader(std::shared_ptr<RenderBackend::PixelShader> pPixelShader);
		
		void Execute(NodeJobType&& job);

	private:
		
		std::string													m_NodeName = "Unnamed node";

		std::vector<GraphResourceHandle>		            		m_InputResources;
		std::vector<GraphResourceHandle>		            		m_OutputResources;
		std::vector<RenderBackend::ERenderResourceState>			m_InputResourceStates;
		std::vector<RenderBackend::ERenderResourceState>			m_OutputResourceStates;

		// TODO: separate graphic pipeline payload from GraphNode
		std::vector<GraphResourceHandle>							m_ColorAttachments;
		std::vector<RenderBackend::RenderPassRenderTargetBinding>	m_ColorAttachmentBindings;
		std::optional<GraphResourceHandle>							m_DepthStencilAttachment;
		RenderBackend::RenderPassRenderTargetBinding				m_DepthStencilAttachmentBinding;
		std::shared_ptr<RenderBackend::VertexShader>				m_VertexShader = nullptr;
		std::shared_ptr<RenderBackend::PixelShader>					m_PixelShader = nullptr;

		std::vector<GraphNode*>					            		m_PrecedeNodes;
		std::vector<GraphNode*>					            		m_SucceedNodes;
		
		NodeJobType													m_Job;

		RenderGraph*												m_RenderGraph = nullptr;
	};

	class RenderGraphNodeMemoryAllocator
	{
		// TODO: align to cache line size to avoid false sharing for multi-threading
		constexpr static auto kChunkMemoryAlignment = 8u;
		constexpr static auto kChunkMemorySizeInByte = 4u * 1024u;

		static_assert(kChunkMemorySizeInByte % kChunkMemoryAlignment == 0);

		struct Chunk
		{
			unsigned char			m_Pad[kChunkMemorySizeInByte];
		};

	public:

		RenderGraphNodeMemoryAllocator() = default;
		~RenderGraphNodeMemoryAllocator();

		RenderGraphNodeMemoryAllocator(const RenderGraphNodeMemoryAllocator&) = delete;
		RenderGraphNodeMemoryAllocator& operator=(const RenderGraphNodeMemoryAllocator&) = delete;
		RenderGraphNodeMemoryAllocator(RenderGraphNodeMemoryAllocator&&) = delete;
		RenderGraphNodeMemoryAllocator& operator=(RenderGraphNodeMemoryAllocator&&) = delete;

		template <typename T>
		T* Allocate();

		/* Release all memories allocated from allocator.
		*  Should only be called when all memories become untouchable by user.
		*/
		void Release();

	private:
		
		std::vector<Chunk*>			m_Chunks;
		std::vector<uint32_t>		m_ChunkLeftOverMemorySizeInByte;
	};
	
	template <typename T>
	T* RenderGraphNodeMemoryAllocator::Allocate()
	{
		// TODO: large memory allocation support
		static_assert(sizeof(T) <= kChunkMemorySizeInByte);

		const uint32_t allocateSize = sizeof(T);
		const uint32_t allocateSizeAligned = Math::AlignTo(allocateSize, kChunkMemoryAlignment);
		
		uint32_t chunkIndex = 0;
		for (auto& leftOverMemory : m_ChunkLeftOverMemorySizeInByte)
		{
			// Have enough memory to allocate within this chunk
			if (leftOverMemory >= allocateSizeAligned)
			{
				const uint32_t chunkMemoryAddressOffset = kChunkMemorySizeInByte - leftOverMemory;

				Chunk* pChunk = m_Chunks[chunkIndex];
				T* pAllocatedMemory = new (reinterpret_cast<volatile unsigned char*>(pChunk) + chunkMemoryAddressOffset) T();

				leftOverMemory -= allocateSizeAligned;
				return pAllocatedMemory;
			
			}
			
			++chunkIndex;
		}

		auto* pChunk = new Chunk;
		m_Chunks.push_back(pChunk);
		auto& leftOverMemory = m_ChunkLeftOverMemorySizeInByte.emplace_back(kChunkMemorySizeInByte);

		leftOverMemory -= allocateSizeAligned;
		
		T* pAllocatedMemory = new (pChunk) T();
		return pAllocatedMemory;
	}

	template <typename T>
	concept IsValidNodeResourceType = Core::IsDefaultConstrctible<T> && Core::IsTriviallyDestructible<T>;

	class RenderGraph : protected GraphNode
	{
		friend class GraphNode;
		friend class GraphResourceHandle;
		friend class GraphExecutionContext;

	public:

		template <GraphResourceType Type>
		using GraphResourceUnderlyingType = typename GraphResourceTrait<Type>::ResourceType;

		template <GraphResourceType Type>
		using GraphResourceStorageType = typename GraphResourceTrait<Type>::ResourceStorageType;

		template <GraphResourceType Type>
		using GraphResourceDescType = typename GraphResourceTrait<Type>::ResourceDescType;

		// TODO: remove dependency of RenderDevice in the constructor
		explicit RenderGraph(RenderBackend::RenderDevice& renderDevice);
		~RenderGraph();

		RenderGraph(const RenderGraph&) = delete;
		RenderGraph& operator=(const RenderGraph&) = delete;
		RenderGraph(RenderGraph&&) = delete;
		RenderGraph& operator=(RenderGraph&&) = delete;

		template <ValidUnderlyingGraphResource T, typename... Args>
		[[nodiscard("Allocated graph resource must be used.")]] GraphResourceHandle CreateResource(const T& desc, Args... args);

		template <typename T>
		[[nodiscard("Imported graph resource must be used.")]] GraphResourceHandle ImportResource(const std::shared_ptr<T>& resource, RenderBackend::ERenderResourceState currentState);

		template <GraphResourceType Type>
		GraphResourceDescType<Type> GetResourceDesc(const GraphResourceHandle& handle) const;
		
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
		void Execute(RenderBackend::PipelineStateCache& pipelineStateCache, RenderBackend::RenderWindow& renderWindow);

	private:

		/* Build render graph by node dependencies.
		*  Sequential execution nodes will be produced.
		*  Then this render graph can be executed.
		*/
		void Build();
		void TopologySort();
		
		const GraphResource& GetResource(const GraphResourceHandle& handle) const;
		uint32_t GetResourceIndex(const GraphResourceHandle& handle) const;

		bool IsResourceStateChanged(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState state) const;
		RenderBackend::ERenderResourceState GetResourceState(const GraphResourceHandle& handle) const;
		void UpdateResourceState(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState state);

		void TransitionResource(const GraphResourceHandle& handle, RenderBackend::ERenderResourceState dstResourceState, uint32_t queueIndex);
		
		// static GraphResourceHandle AllocateResourceHandle(const GraphResource& graphResource);

	private:

		std::reference_wrapper<RenderBackend::RenderDevice>		m_RenderDevice;

		GraphNode*												m_TailNode = this;
		std::unordered_map<std::string, GraphNode>				m_GraphNodes;

		std::vector<GraphNode*>									m_ExecutionNodes;
	
		std::unordered_map<std::string, void*>					m_AllocatedMemory;
		RenderGraphNodeMemoryAllocator							m_Allocator;

		std::vector<GraphResource>								m_Resources;
		std::vector<RenderBackend::ERenderResourceState>		m_CurrentResourcesStates;
	};

	template <ValidUnderlyingGraphResource T, typename... Args>
	GraphResourceHandle RenderGraph::CreateResource(const T& desc, Args... args)
	{
		constexpr GraphResourceType resourceType = GraphUnderlyingResourceTrait<T>::type;
		// TODO: defer create this
		auto pResource = std::shared_ptr<GraphResourceUnderlyingType<resourceType>>(GraphResourceUnderlyingType<resourceType>::Create(m_RenderDevice, desc, std::forward<Args>(args)...));
		const auto resourceIndex = static_cast<uint32_t>(m_Resources.size());
		m_Resources.emplace_back(pResource);
		m_CurrentResourcesStates.push_back(RenderBackend::ERenderResourceState::Undefined);
		
		auto& graphResource = m_Resources.back();
		// graphResource.m_ResourceIndex = resourceIndex;
		m_RenderDevice.get().DeferRelease(graphResource.GetResourceStorage<resourceType>());

		GraphResourceHandle handle;
		handle.m_ResourceId = resourceIndex;
		return handle;
	}

	template<typename T>
	GraphResourceHandle RenderGraph::ImportResource(const std::shared_ptr<T>& resource, RenderBackend::ERenderResourceState currentState)
	{
		static_assert(ValidUnderlyingGraphResource<T>);
		const auto resourceIndex = static_cast<uint32_t>(m_Resources.size()); 
		m_Resources.emplace_back(resource);
		m_CurrentResourcesStates.push_back(currentState);
		// auto& graphResource = m_Resources.back();
		// graphResource.m_ResourceIndex = resourceIndex;

		GraphResourceHandle handle;
		handle.m_ResourceId = resourceIndex;
		return handle;
	}
	
	template <GraphResourceType Type>
	RenderGraph::GraphResourceDescType<Type> RenderGraph::GetResourceDesc(const GraphResourceHandle& handle) const
	{
		return GetResource(handle).GetDesc<Type>();
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

	class GraphExecutionContext
	{
		friend class RenderGraph;
		
	public:

		template <GraphResourceType Type>
		auto GetDesc(const GraphResourceHandle& resourceHandle) const;
		
		void SetViewportSize(uint32_t width, uint32_t height) const { SetViewportSize({width, height}); }
		void SetViewportSize(const glm::uvec2& viewportSize) const;

		void BindVertexInput(const GraphResourceHandle& vertexBufferHandle, const GraphResourceHandle& indexBufferHandle = {}) const;
		
		template <typename T>
		void UpdateUniformBuffer(const GraphResourceHandle& handle, const T& data);

		void BindResource(const std::string& name, const GraphResourceHandle& handle);
		void BindPipeline();

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const;
		
	private:

		GraphExecutionContext(RenderGraph& renderGraph);
		~GraphExecutionContext() = default;

		void SetPipeline(RenderBackend::PipelineState* pPipelineState, VkPipelineBindPoint bindPoint)
		{
			m_PipelineState = pPipelineState;
			m_PipelineBindPoint = bindPoint;
		}

		void SetDescriptorSets(const std::vector<VkDescriptorSet>& sets)
		{
			m_DescriptorSets = &sets;
		}

		void SetCommandList(RenderBackend::RenderCommandList& commandList)
		{
			m_RenderCommandList = &commandList;
		}

		void SetRenderTargets(std::vector<RenderBackend::Texture*>* pRenderTargetPtrs, std::vector<RenderBackend::RenderPassRenderTargetBinding>* pRenderTargetBindings)
		{
			m_RenderTargetPtrs = pRenderTargetPtrs;
			m_RenderTargetBindings = pRenderTargetBindings;
		}
		
	private:

		std::reference_wrapper<RenderGraph>					m_RenderGraph;
		RenderBackend::RenderCommandList*					m_RenderCommandList = nullptr;

		std::vector<RenderBackend::Texture*>*							m_RenderTargetPtrs = nullptr;
		std::vector<RenderBackend::RenderPassRenderTargetBinding>*		m_RenderTargetBindings = nullptr;

		RenderBackend::PipelineState*						m_PipelineState = nullptr;
		const std::vector<VkDescriptorSet>*					m_DescriptorSets = nullptr;
		VkPipelineBindPoint									m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		
		std::vector<std::vector<VkWriteDescriptorSet>>		m_WriteDescriptorSets;
		// TODO: replace to inline linked list
		std::forward_list<VkDescriptorBufferInfo>			m_TemporaryBufferInfos;
		std::forward_list<VkDescriptorImageInfo>			m_TemporaryImageInfos;
	};

	template <GraphResourceType Type>
	auto GraphExecutionContext::GetDesc(const GraphResourceHandle& resourceHandle) const
	{
		return m_RenderGraph.get().GetResourceDesc<Type>(resourceHandle);
	}
	
	template <typename T>
	void GraphExecutionContext::UpdateUniformBuffer(const GraphResourceHandle& handle, const T& data)
	{
		auto& renderResource = m_RenderGraph.get().GetResource(handle);
		if (renderResource.IsTypeOf<GraphResourceType::Buffer>() && (renderResource.GetDesc<GraphResourceType::Buffer>().m_Usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0)
		{
			auto scope = renderResource.GetResourceStorage<GraphResourceType::Buffer>()->Map();
			memcpy(scope.m_pMappedMemory, &data, sizeof(T)); 
		}
		else
		{
			ZE_LOG_WARNING("Try to update a non uniform buffer resource in UpdateUniformBuffer().");
		}
	}
}
