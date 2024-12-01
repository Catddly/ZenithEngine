#include "RenderGraphResource.h"

namespace ZE::RenderGraph
{
	GraphResource::GraphResource(const BufferResource& bufferResource)
		: m_GraphResourceInterface(bufferResource)
	{
	}

	GraphResource::GraphResource(BufferResource&& bufferResource)
		: m_GraphResourceInterface(bufferResource)
	{
	}

	GraphResource::GraphResource(const TextureResource& textureResource)
		: m_GraphResourceInterface(textureResource)
	{
	}

	GraphResource::GraphResource(TextureResource&& textureResource)
		: m_GraphResourceInterface(textureResource)
	{
	}
}
