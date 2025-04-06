#pragma once

#include <memory>

namespace ZE::Render
{
	class RenderGraph;
	class GraphResourceHandle;
}

namespace ZE::RenderBackend
{
	class VertexShader;
	class PixelShader;
	class RenderDevice;
	class Buffer;
}

namespace ZE::Renderer
{
	class TriangleRenderer
	{
	public:

		bool Prepare(RenderBackend::RenderDevice& renderDevice);
		void Release(RenderBackend::RenderDevice& renderDevice);
		
		void Render(Render::RenderGraph& renderGraph, Render::GraphResourceHandle outputColorRT, Render::GraphResourceHandle outputDepthRT);
	
	private:

		std::shared_ptr<RenderBackend::VertexShader>		m_TriangleVS = nullptr;
		std::shared_ptr<RenderBackend::PixelShader>			m_TrianglePS = nullptr;

		std::shared_ptr<RenderBackend::Buffer>				m_VertexBuffer = nullptr;			
		std::shared_ptr<RenderBackend::Buffer>				m_IndexBuffer = nullptr;			
	};
}
