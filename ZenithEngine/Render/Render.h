#pragma once

#include "Core/Module.h"
#include "Renderer/TriangleRenderer.h"

#include <memory>

namespace ZE::RenderBackend
{
	class PipelineStateCache;
	class RenderDevice; class RenderWindow;
	class VertexShader; class PixelShader;
}

namespace ZE::Render
{
	class RenderModule : public Core::IModule
	{
	public:

		RenderModule(Core::Engine& engine)
			: IModule(engine, Core::EModuleInitializePhase::Init, "Render")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		void Render();
		
		std::shared_ptr<RenderBackend::RenderWindow> GetMainRenderWindow() const { return m_MainRenderWindow; }

	private:

		RenderBackend::RenderDevice*					m_RenderDevice = nullptr;
		RenderBackend::PipelineStateCache*				m_PipelineStateCache = nullptr;

		std::shared_ptr<RenderBackend::RenderWindow>	m_MainRenderWindow = nullptr;

		Renderer::TriangleRenderer						m_TriangleRenderer;
	};
}