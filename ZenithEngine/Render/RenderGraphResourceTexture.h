#pragma once

#include "RenderGraphResource.h"

#include <vector>

namespace ZE::RenderGraph
{
	// Dev purpose only
	struct TextureResource
	{
		std::vector<int>	m_SerialNumberArray;
	};

	template <>
	class GraphResourceInterface<GraphResourceType::Texture>
	{
	public:


	private:

		TextureResource					m_Resource;
	};
}