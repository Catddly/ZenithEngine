#include "RenderGraphResource.h"

namespace ZE::Render
{
	GraphResource::GraphResource(const GraphResourceTraitResourceType<GraphResourceType::Buffer>& resource)
		: m_GraphResourceInterface(resource)
	{

	}

	GraphResource::GraphResource(GraphResourceTraitResourceType<GraphResourceType::Buffer>&& resource)
		: m_GraphResourceInterface(resource)
	{
	}

	GraphResource::GraphResource(const GraphResourceTraitResourceType<GraphResourceType::Texture>& resource)
		: m_GraphResourceInterface(resource)
	{

	}

	GraphResource::GraphResource(GraphResourceTraitResourceType<GraphResourceType::Texture>&& resource)
		: m_GraphResourceInterface(resource)
	{
	}
}
