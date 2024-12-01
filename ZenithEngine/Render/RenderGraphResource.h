#pragma once

//#include <concepts>
#include <variant>
#include <vector>

namespace ZE::RenderGraph
{
	enum class GraphResourceType
	{
		Buffer,
		Texture
	};

	template <GraphResourceType Type>
	class GraphResourceTrait
	{
	};

	// Dev purpose only
	struct BufferResource
	{
		int						m_Id;
		double					m_Value;

		static constexpr GraphResourceType ResourceGraphResourceType = GraphResourceType::Buffer;
	};

	// Dev purpose only
	struct TextureResource
	{
		std::vector<int>		m_SerialNumberArray;

		static constexpr GraphResourceType ResourceGraphResourceType = GraphResourceType::Texture;
	};

	template <>
	class GraphResourceTrait<GraphResourceType::Buffer>
	{
	public:

		using ResourceType = BufferResource;
	};

	template <>
	class GraphResourceTrait<GraphResourceType::Texture>
	{
	public:

		using ResourceType = TextureResource;
	};

	template <GraphResourceType T>
	concept ValidGraphResourceType = requires
	{
		typename GraphResourceTrait<T>::ResourceType;
	};

	//-------------------------------------------------------------------------

	template <GraphResourceType Type>
	class GraphResourceInterface
	{
		static_assert(ValidGraphResourceType<Type>);

	public:

		//GraphResourceInterface() = default;
		GraphResourceInterface(typename GraphResourceTrait<Type>::ResourceType inResource)
			: m_Resource(inResource)
		{}

	private:

		GraphResourceTrait<Type>::ResourceType						m_Resource;
	};

	//template <>
	//class GraphResourceInterface<GraphResourceType::Texture>
	//{
	//public:

	//private:

	//	TextureResource					m_Resource;
	//};

	//template <typename T>
	//concept CanConstructGraphResource = requires
	//{
	// 	requires std::same_as<T, BufferResource> || std::same_as<T, TextureResource>;
	//	requires T::ResourceGraphResourceType;
	//};

	class GraphResource
	{
		using GraphResourceInterfaceType = std::variant<
			GraphResourceInterface<GraphResourceType::Buffer>,
			GraphResourceInterface<GraphResourceType::Texture>
		>;

		template <GraphResourceType Type>
		using GraphResourceTraitResourceType = typename GraphResourceTrait<Type>::ResourceType;

	public:

		GraphResource(const BufferResource& bufferResource);
		GraphResource(BufferResource&& bufferResource);
		GraphResource(const TextureResource& textureResource);
		GraphResource(TextureResource&& textureResource);

	private:

		GraphResourceInterfaceType							m_GraphResourceInterface;
	};
}