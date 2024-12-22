#pragma once

#include "ModuleDefines.h"
#include "Core/Module.h"

#include "taskflow/taskflow.hpp"

#include <memory>

namespace ZE::Core { class CoreModule; }
namespace ZE::Log { class LogModule; }
namespace ZE::Render { class RenderModule; }
namespace ZE::Platform { class IDisplayable; class Window; }

namespace ZE::Engine
{
	class ENGINE_API ZenithEngine
	{
	public:

		virtual ~ZenithEngine() = default;

		virtual bool PreInitialize();
		virtual bool Initialize();
		virtual void Shutdown();
		virtual void PostShutdown();

		virtual void Run();

		//-------------------------------------------------------------------------

		Core::CoreModule* GetCoreModule() const { return m_pCoreModule; }
		Log::LogModule* GetLogModule() const { return m_pLogModule; }
		Render::RenderModule* GetRenderModule() const { return m_pRenderModule; }

	protected:

		/* Build frame tasks in user customize order and priority. */
		virtual void BuildFrameTasks(tf::Taskflow& taskFlow);

	private:

		/* Clear frame tasks. 
		*  TODO: may be cached frame tasks?
		*/
		void ClearFrameTasks();

		bool PreInitializeModule(Core::IModule* pModule);
		bool InitializeModule(Core::IModule* pModule);

	protected:

		Core::CoreModule*					m_pCoreModule = nullptr;
		Log::LogModule*						m_pLogModule = nullptr;

		Render::RenderModule*				m_pRenderModule = nullptr;

	private:

		tf::Taskflow					m_TaskFlow = { "Engine" };
		tf::Executor					m_TaskExecutor;

		bool							m_bIsPreInitialized = false;
		bool							m_bIsInitialized = false;
		bool							m_RequestExit = false;
	};
}
