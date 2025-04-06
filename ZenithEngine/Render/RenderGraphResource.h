#pragma once

#include "RenderBackend/RenderResource.h"

#include <variant>

namespace ZE::Render
{
	enum class GraphResourceType
	{
		Buffer,
		Texture
	};

	template <typename T>
	struct GraphUnderlyingResourceTrait {};

	template <>
	struct GraphUnderlyingResourceTrait<RenderBackend::BufferDesc>
	{
		constexpr static GraphResourceType type = GraphResourceType::Buffer;
	};

	template <>
	struct GraphUnderlyingResourceTrait<RenderBackend::TextureDesc>
	{
		constexpr static GraphResourceType type = GraphResourceType::Texture;
	};

	template <>
	struct GraphUnderlyingResourceTrait<RenderBackend::Buffer>
	{
		constexpr static GraphResourceType type = GraphResourceType::Buffer;
	};

	template <>
	struct GraphUnderlyingResourceTrait<RenderBackend::Texture>
	{
		constexpr static GraphResourceType type = GraphResourceType::Texture;
	};

	template <typename T>
	concept ValidUnderlyingGraphResource = requires
	{
		GraphUnderlyingResourceTrait<T>::type;
	};

	template <GraphResourceType Type>
	class GraphResourceTrait {};

	template <>
	class GraphResourceTrait<GraphResourceType::Buffer>
	{
	public:
		using ResourceType = RenderBackend::Buffer;
		using ResourceStorageType = RenderBackend::DeferReleaseLifetimeResource<RenderBackend::Buffer>;
		using ResourceDescType = RenderBackend::BufferDesc;
	};

	template <>
	class GraphResourceTrait<GraphResourceType::Texture>
	{
	public:
		using ResourceType = RenderBackend::Texture;
		using ResourceStorageType = RenderBackend::DeferReleaseLifetimeResource<RenderBackend::Texture>;
		using ResourceDescType = RenderBackend::TextureDesc;
	};

	template <GraphResourceType T>
	concept ValidGraphResourceType = requires
	{
		typename GraphResourceTrait<T>::ResourceType;
		typename GraphResourceTrait<T>::ResourceStorageType;
		typename GraphResourceTrait<T>::ResourceDescType;
	};

	//-------------------------------------------------------------------------

	template <GraphResourceType Type>
	class GraphResourceInterface
	{
		static_assert(ValidGraphResourceType<Type>);

	public:

		using GraphResourceTraitResourceStorageType = typename GraphResourceTrait<Type>::ResourceStorageType;

		GraphResourceInterface(const GraphResourceTraitResourceStorageType& inResource)
			: m_Resource(inResource)
		{}
		GraphResourceInterface(GraphResourceTraitResourceStorageType&& inResource)
			: m_Resource(std::move(inResource))
		{}

		inline const typename GraphResourceTrait<Type>::ResourceDescType& GetDesc() const
		{
			return m_Resource->GetDesc();
		}

		inline const typename GraphResourceTrait<Type>::ResourceStorageType& GetResourceStorage() const
		{
			return m_Resource;
		}
		
	private:

		typename GraphResourceTrait<Type>::ResourceStorageType				m_Resource;
	};

	/* Lightweight resource pointer type points to an underlying resource.
	*  This type will be frequently copy back and forth. 
	*/
	class GraphResource
	{
		friend class GraphNode;
		friend class RenderGraph;
		
		using GraphResourceInterfaceType = std::variant<
			GraphResourceInterface<GraphResourceType::Buffer>,
			GraphResourceInterface<GraphResourceType::Texture>
		>;

		template <GraphResourceType Type>
		using GraphResourceTraitResourceStorageType = typename GraphResourceTrait<Type>::ResourceStorageType;

		template <GraphResourceType Type>
		using GraphResourceTraitResourceDescType = typename GraphResourceTrait<Type>::ResourceDescType;

	public:
		
		GraphResource(const GraphResourceTraitResourceStorageType<GraphResourceType::Buffer>& resource);
		GraphResource(GraphResourceTraitResourceStorageType<GraphResourceType::Buffer>&& resource);
		GraphResource(const GraphResourceTraitResourceStorageType<GraphResourceType::Texture>& resource);
		GraphResource(GraphResourceTraitResourceStorageType<GraphResourceType::Texture>&& resource);

		template <GraphResourceType Type>
		constexpr bool IsTypeOf() const
		{
			return std::holds_alternative<GraphResourceInterface<Type>>(m_GraphResourceInterface);
		}

		template <GraphResourceType Type>
		const GraphResourceTraitResourceDescType<Type>& GetDesc() const
		{
			return std::get<GraphResourceInterface<Type>>(m_GraphResourceInterface).GetDesc();
		}

		template <GraphResourceType Type>
		const GraphResourceTraitResourceStorageType<Type>& GetResourceStorage() const
		{
			return std::get<GraphResourceInterface<Type>>(m_GraphResourceInterface).GetResourceStorage();
		}

	private:

		GraphResourceInterfaceType							m_GraphResourceInterface;
	};
}