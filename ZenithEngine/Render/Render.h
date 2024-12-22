#pragma once

#include "Core/Module.h"

#include <memory>

namespace ZE::RenderBackend { class IRenderDevice; class RenderWindow; }

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

		std::shared_ptr<RenderBackend::RenderWindow> GetMainRenderWindow() const { return m_pMainRenderWindow; }

		//inline std::shared_ptr<RenderBackend::RenderWindow> CreateSecondaryRenderWindow();
	
	private:

		RenderBackend::IRenderDevice*					m_pRenderDevice = nullptr;

		std::shared_ptr<RenderBackend::RenderWindow>	m_pMainRenderWindow = nullptr;
	};
}