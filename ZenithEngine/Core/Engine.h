#pragma once

#include "ModuleDefines.h"
#include "Core/Module.h"

namespace ZE::Core { class CoreModule; }
namespace ZE::Log { class LogModule; }
namespace ZE::Input { class InputModule; }
namespace ZE::Render { class RenderModule; }
namespace ZE::Platform { class IDisplayable; class Window; }

namespace ZE::Core
{
	class ENGINE_API Engine
	{
	public:

		virtual ~Engine() = default;

		virtual bool PreInitialize();
		virtual bool Initialize();
		virtual void Shutdown();
		virtual void PostShutdown();

		virtual void Run();

		//-------------------------------------------------------------------------

		CoreModule* GetCoreModule() const { return m_CoreModule; }
		Input::InputModule* GetInputModule() const { return m_InputModule; }
		Log::LogModule* GetLogModule() const { return m_LogModule; }
		Render::RenderModule* GetRenderModule() const { return m_RenderModule; }
	
	private:
		
		static bool InitializeModule(Core::IModule* pModule);
		
	protected:

		CoreModule*						m_CoreModule = nullptr;
		Input::InputModule*				m_InputModule = nullptr;
		Log::LogModule*					m_LogModule = nullptr;

		Render::RenderModule*			m_RenderModule = nullptr;

	private:
		
		bool							m_IsPreInitialized = false;
		bool							m_IsInitialized = false;
		bool							m_RequestExit = false;
	};
}
