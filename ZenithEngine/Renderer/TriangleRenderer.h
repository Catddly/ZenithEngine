#pragma once

#include <memory>

#include "Render/StaticMesh.h"
#include "Render/Shader.h"

namespace ZE::Render
{
	class RenderGraph;
	class GraphResourceHandle;
}

namespace ZE::RenderBackend
{
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

		Asset::AssetPtr<Render::VertexShader>				m_TriangleVS{"Content/Shader/Triangle.vsdr"};
		Asset::AssetPtr<Render::PixelShader>				m_TrianglePS{"Content/Shader/Triangle.psdr"};

		// TODO: make static mesh asset
		std::shared_ptr<RenderBackend::Buffer>				m_VertexBuffer = nullptr;			
		std::shared_ptr<RenderBackend::Buffer>				m_IndexBuffer = nullptr;

		Asset::AssetPtr<Render::StaticMesh>					m_TestAsset{"Content/Mesh/Cerberus/scene.gltf"};
	};
}
