#include "Engine.h"

#include "Core/Assertion.h"
#include "Core/Core.h"
#include "Log/Log.h"
#include "Render/Render.h"
#include "Platform/Window.h"
#include "Platform/Displayable.h"
#include "RenderBackend/RenderWindow.h"

#include "GLFW/glfw3.h"

namespace ZE::Engine
{
	bool ZenithEngine::PreInitialize()
	{
		ZE_CHECK(!m_bIsPreInitialized);

		m_pLogModule = new Log::LogModule(*this);
		if (!PreInitializeModule(m_pLogModule))
		{
			return false;
		}

		m_pCoreModule = new Core::CoreModule(*this);
		if (!PreInitializeModule(m_pCoreModule))
		{
			return false;
		}

		m_bIsPreInitialized = true;
		return m_bIsPreInitialized;
	}

	bool ZenithEngine::Initialize()
	{
		ZE_CHECK(!m_bIsInitialized);

		m_pRenderModule = new Render::RenderModule(*this);
		if (!InitializeModule(m_pRenderModule))
		{
			return false;
		}

		m_bIsInitialized = true;
		return m_bIsInitialized;
	}

	void ZenithEngine::Shutdown()
	{
		ZE_CHECK(m_bIsInitialized);

		m_pRenderModule->ShutdownModule();
		delete m_pRenderModule;
	}

	void ZenithEngine::PostShutdown()
	{
		ZE_CHECK(m_bIsPreInitialized);

		m_pCoreModule->ShutdownModule();
		delete m_pCoreModule;

		m_pLogModule->ShutdownModule();
		delete m_pLogModule;
	}

	void ZenithEngine::Run()
	{
		ZE_CHECK(m_bIsPreInitialized);
		ZE_CHECK(m_bIsInitialized);

		while (!m_RequestExit)
		{
			BuildFrameTasks(m_TaskFlow);

			// TODO: this thread should work too.
			m_TaskExecutor.run(m_TaskFlow).wait();
			
			ClearFrameTasks();
		}
	}

	//-------------------------------------------------------------------------

	void ZenithEngine::BuildFrameTasks(tf::Taskflow& taskFlow)
	{
		ZE_CHECK(taskFlow.empty());

		auto coreTask = taskFlow.composed_of(m_pCoreModule->BuildAndGetFrameTasks()).name(m_pCoreModule->GetModuleName());
		auto logTask = taskFlow.composed_of(m_pLogModule->BuildAndGetFrameTasks()).name(m_pLogModule->GetModuleName());

		auto renderTask = taskFlow.composed_of(m_pRenderModule->BuildAndGetFrameTasks()).name(m_pRenderModule->GetModuleName());
	
		// Build task dependencies

		coreTask.precede(logTask);
		logTask.succeed(renderTask);
	}

	void ZenithEngine::ClearFrameTasks()
	{
		m_pLogModule->ClearFrameTasks();
		m_pCoreModule->ClearFrameTasks();

		m_pRenderModule->ClearFrameTasks();

		m_TaskFlow.clear();
	}

	bool ZenithEngine::PreInitializeModule(Core::IModule* pModule)
	{
		if (!pModule->InitializeModule())
		{
			ZE_LOG_FATAL("Zenith engine failed to pre-initialized module [{}].", pModule->GetModuleName());
			pModule->ShutdownModule();
			return false;
		}

		ZE_LOG_INFO("Module [{}] is initialized.", pModule->GetModuleName());
		return true;
	}

	bool ZenithEngine::InitializeModule(Core::IModule* pModule)
	{
		if (!pModule->InitializeModule())
		{
			ZE_LOG_FATAL("Zenith engine failed to initialized module [{}].", pModule->GetModuleName());
			pModule->ShutdownModule();
			return false;
		}

		ZE_LOG_INFO("Module [{}] is initialized.", pModule->GetModuleName());
		return true;
	}
}
