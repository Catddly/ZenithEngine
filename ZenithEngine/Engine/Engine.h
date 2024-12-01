#pragma once

#include "ModuleDefines.h"
#include "Core/Module.h"

#include "taskflow/taskflow.hpp"

namespace ZE::Core { class CoreModule; }
namespace ZE::Log { class LogModule; }
namespace ZE::Render { class RenderModule; }

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

	protected:

		/* Build frame tasks in user customize order and priority. */
		virtual void BuildFrameTasks(tf::Taskflow& taskFlow);

	private:

		/* Clear frame tasks. 
		*  TODO: may be cached frame tasks?
		*/
		void ClearFrameTasks();

		bool PreinitializeModule(Core::IModule* pModule);
		bool InitializeModule(Core::IModule* pModule);

	protected:

		Core::CoreModule*			m_pCoreModule = nullptr;
		Log::LogModule*				m_pLogModule = nullptr;

		Render::RenderModule*		m_pRenderModule = nullptr;

	private:

		tf::Taskflow				m_TaskFlow = { "Engine" };
		tf::Executor				m_TaskExecutor;

		bool						m_bIsPreInitialized = false;
		bool						m_bIsInitialized = false;
		bool						m_RequestExit = false;
	};
}
