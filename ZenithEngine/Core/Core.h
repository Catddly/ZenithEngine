#pragma once

#include "Core/Module.h"

namespace ZE::Platform { class IDisplayable; class Window; }

namespace ZE::Core
{
	class CoreModule : public IModule
	{
	public:

		CoreModule(Engine& engine)
			: IModule(engine, EModuleInitializePhase::PreInit, "Core")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;
		
		void ProcessPlatformEvents();
		
	private:

		Platform::IDisplayable*				m_DisplayDevice = nullptr;
	};
}
