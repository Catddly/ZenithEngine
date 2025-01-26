#pragma once

#include "Core/Module.h"

#include <memory>

namespace ZE::RenderBackend { class RenderDevice; class RenderWindow; class VertexShader; class PixelShader; }

namespace ZE::Render
{

	class RenderModule : public Core::IModule
	{
	public:

		RenderModule(Engine::ZenithEngine& engine)
			: Core::IModule(engine, Core::ModuleInitializePhase::Init, "Render")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) override;

	public:

		std::shared_ptr<RenderBackend::RenderWindow> GetMainRenderWindow() const { return m_pMainRenderWindow; }

		//inline std::shared_ptr<RenderBackend::RenderWindow> CreateSecondaryRenderWindow();
	
	private:

		void Draw();

	private:

		RenderBackend::RenderDevice*					m_pRenderDevice = nullptr;

		std::shared_ptr<RenderBackend::RenderWindow>	m_pMainRenderWindow = nullptr;

		// Temp
		std::shared_ptr<RenderBackend::VertexShader>	m_pTriangleVS = nullptr;
		std::shared_ptr<RenderBackend::PixelShader>		m_pTrianglePS = nullptr;
	};
}