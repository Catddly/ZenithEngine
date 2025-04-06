#include "RenderGraphResource.h"

namespace ZE::Render
{
	GraphResource::GraphResource(const GraphResourceTraitResourceStorageType<GraphResourceType::Buffer>& resource)
		: m_GraphResourceInterface(resource)
	{}

	GraphResource::GraphResource(GraphResourceTraitResourceStorageType<GraphResourceType::Buffer>&& resource)
		: m_GraphResourceInterface(std::move(resource))
	{}

	GraphResource::GraphResource(const GraphResourceTraitResourceStorageType<GraphResourceType::Texture>& resource)
		: m_GraphResourceInterface(resource)
	{}

	GraphResource::GraphResource(GraphResourceTraitResourceStorageType<GraphResourceType::Texture>&& resource)
		: m_GraphResourceInterface(std::move(resource))
	{}
}
