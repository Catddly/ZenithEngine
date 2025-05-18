#pragma once

#include "Core/Module.h"

namespace ZE::Platform { class Window; }
namespace ZE::Input
{
	class ISystemInputCollector;
	
	class InputModule : public Core::IModule
	{
	public:

		InputModule(Core::Engine& engine)
			: IModule(engine, Core::EModuleInitializePhase::PreInit, "Input")
		{}

		void CollectSystemInputs(ISystemInputCollector* pInputCollector);

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		void AddWindow(Platform::Window* pWindow);
		void RemoveWindow(Platform::Window* pWindow);

	private:

		ISystemInputCollector*					m_SystemInputCollector = nullptr;
			
		bool									m_IsInitialized = false;
	};
}
