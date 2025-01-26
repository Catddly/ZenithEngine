#pragma once

#include "RenderBackend/RenderResource.h"

#include <variant>
#include <vector>
#include <memory>

namespace ZE::Render
{
	enum class GraphResourceType
	{
		Buffer,
		Texture
	};

	template <typename T>
	struct GraphUnderlyingResourceTarit {};

	template <>
	struct GraphUnderlyingResourceTarit<RenderBackend::BufferDesc>
	{
		constexpr static GraphResourceType type = GraphResourceType::Buffer;
	};

	template <>
	struct GraphUnderlyingResourceTarit<RenderBackend::TextureDesc>
	{
		constexpr static GraphResourceType type = GraphResourceType::Texture;
	};

	template <>
	struct GraphUnderlyingResourceTarit<RenderBackend::Buffer>
	{
		constexpr static GraphResourceType type = GraphResourceType::Buffer;
	};

	template <>
	struct GraphUnderlyingResourceTarit<RenderBackend::Texture>
	{
		constexpr static GraphResourceType type = GraphResourceType::Texture;
	};

	template <typename T>
	concept ValidUnderlyingGraphResource = requires
	{
		GraphUnderlyingResourceTarit<T>::type;
	};

	template <GraphResourceType Type>
	class GraphResourceTrait {};

	template <>
	class GraphResourceTrait<GraphResourceType::Buffer>
	{
	public:
		using ResourceType = RenderBackend::Buffer;
		using ResourceStorageType = std::shared_ptr<RenderBackend::Buffer>;
		using ResourceDescType = RenderBackend::BufferDesc;
	};

	template <>
	class GraphResourceTrait<GraphResourceType::Texture>
	{
	public:
		using ResourceType = RenderBackend::Texture;
		using ResourceStorageType = std::shared_ptr<RenderBackend::Texture>;
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
			: m_Resource(inResource)
		{}

	private:

		GraphResourceTrait<Type>::ResourceStorageType						m_Resource;
	};

	/* Lightweight resource pointer type points to a underlying resource.
	*  This type will be frequently copy back and forth. 
	*/
	class GraphResource
	{
		using GraphResourceInterfaceType = std::variant<
			GraphResourceInterface<GraphResourceType::Buffer>,
			GraphResourceInterface<GraphResourceType::Texture>
		>;

		template <GraphResourceType Type>
		using GraphResourceTraitResourceType = typename GraphResourceTrait<Type>::ResourceStorageType;

	public:

		GraphResource(const GraphResourceTraitResourceType<GraphResourceType::Buffer>& resource);
		GraphResource(GraphResourceTraitResourceType<GraphResourceType::Buffer>&& resource);
		GraphResource(const GraphResourceTraitResourceType<GraphResourceType::Texture>& resource);
		GraphResource(GraphResourceTraitResourceType<GraphResourceType::Texture>&& resource);

		template <GraphResourceType Type>
		inline bool IsTypeOf() const
		{
			return std::holds_alternative<GraphResourceInterface<Type>>(m_GraphResourceInterface);
		}

	private:

		GraphResourceInterfaceType							m_GraphResourceInterface;
	};
}