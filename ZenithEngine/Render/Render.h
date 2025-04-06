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

		RenderModule(Engine::Engine& engine)
			: Core::IModule(engine, Core::ModuleInitializePhase::Init, "Render")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) override;

	public:

		std::shared_ptr<RenderBackend::RenderWindow> GetMainRenderWindow() const { return m_MainRenderWindow; }

		//inline std::shared_ptr<RenderBackend::RenderWindow> CreateSecondaryRenderWindow();
	
	private:

		void Draw();

	private:

		RenderBackend::RenderDevice*					m_RenderDevice = nullptr;
		RenderBackend::PipelineStateCache*				m_PipelineStateCache = nullptr;

		std::shared_ptr<RenderBackend::RenderWindow>	m_MainRenderWindow = nullptr;

		Renderer::TriangleRenderer						m_TriangleRenderer;

		// Temp
		// std::shared_ptr<RenderBackend::VertexShader>	m_TriangleVS = nullptr;
		// std::shared_ptr<RenderBackend::PixelShader>		m_TrianglePS = nullptr;
	};
}