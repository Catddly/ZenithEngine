#pragma once

#include "RenderGraphResource.h"

namespace ZE::RenderGraph
{
	// Dev purpose only
	struct BufferResource
	{
		int					m_Id;
		double				m_Value;
	};

	template <>
	class GraphResourceInterface<GraphResourceType::Buffer>
	{
	public:


	private:

		BufferResource					m_Resource;
	};
}