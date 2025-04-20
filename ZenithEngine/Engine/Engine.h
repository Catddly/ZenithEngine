#pragma once

#include "ModuleDefines.h"
#include "Core/Module.h"

namespace ZE::Core { class CoreModule; }
namespace ZE::Log { class LogModule; }
namespace ZE::Render { class RenderModule; }
namespace ZE::Platform { class IDisplayable; class Window; }

namespace ZE::Engine
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

		Core::CoreModule* GetCoreModule() const { return m_CoreModule; }
		Log::LogModule* GetLogModule() const { return m_LogModule; }
		Render::RenderModule* GetRenderModule() const { return m_RenderModule; }

		// TaskSystem::TaskManager& GetTaskManager() { return m_TaskManager; }

	private:
		
		static bool InitializeModule(Core::IModule* pModule);

	protected:

		Core::CoreModule*				m_CoreModule = nullptr;
		Log::LogModule*					m_LogModule = nullptr;

		Render::RenderModule*			m_RenderModule = nullptr;

	private:
		
		bool							m_IsPreInitialized = false;
		bool							m_IsInitialized = false;
		bool							m_RequestExit = false;
	};
}
