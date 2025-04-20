#include "Engine.h"

#include "Core/Assertion.h"
#include "Core/Core.h"
#include "Core/Timer.h"
#include "Log/Log.h"
#include "Render/Render.h"
#include "Platform/Window.h"
#include "RenderBackend/RenderWindow.h"
#include "TaskSystem/TaskManager.h"

#include <GLFW/glfw3.h>

#include <numeric>
#include <ranges>

using namespace std::chrono_literals;

namespace ZE::Engine
{
	bool Engine::PreInitialize()
	{
		ZE_CHECK(!m_IsPreInitialized);

		m_LogModule = new Log::LogModule(*this);
		if (!InitializeModule(m_LogModule))
		{
			return false;
		}

		m_CoreModule = new Core::CoreModule(*this);
		if (!InitializeModule(m_CoreModule))
		{
			return false;
		}

		m_IsPreInitialized = true;
		return m_IsPreInitialized;
	}

	bool Engine::Initialize()
	{
		ZE_CHECK(!m_IsInitialized);

		m_RenderModule = new Render::RenderModule(*this);
		if (!InitializeModule(m_RenderModule))
		{
			return false;
		}
		
		m_IsInitialized = true;
		return m_IsInitialized;
	}

	void Engine::Shutdown()
	{
		ZE_CHECK(m_IsInitialized);

		m_RenderModule->ShutdownModule();
		delete m_RenderModule;
	}

	void Engine::PostShutdown()
	{
		ZE_CHECK(m_IsPreInitialized);

		m_CoreModule->ShutdownModule();
		delete m_CoreModule;

		m_LogModule->ShutdownModule();
		delete m_LogModule;
	}
	
	void Engine::Run()
	{
		ZE_CHECK(m_IsPreInitialized);
		ZE_CHECK(m_IsInitialized);

		uint64_t frameCounter = 0ull;
		double frameTimes[20] = {};
		
		TaskSystem::TaskHandle renderTask;
		while (!m_RequestExit)
		{
			{
				Core::ScopedTimer<Core::ETimeUnit::MilliSecond> scopedTimer(frameTimes[frameCounter % 10]);

				renderTask.Wait();
				m_CoreModule->ProcessPlatformEvents();

				if (m_RenderModule->GetMainRenderWindow()->IsRequestingClose())
				{
					m_RequestExit = true;
					break;
				}
				
				renderTask = TaskSystem::TaskManager::Get().RunTask([this, frameCounter]
				{
					m_RenderModule->Render();
				}, {}, TaskSystem::EDedicatedThread::RenderThread);
			}

			if (frameCounter % 1000 == 1 /* skip first 1000 frames */)
			{
				const double averageFrameTime = std::accumulate(frameTimes, frameTimes + std::min(frameCounter, 10ull), 0.0) / static_cast<double>(std::min(frameCounter, 10ull));
				ZE_LOG_INFO("Current frame rate: {:.5} fps", 1000.f / averageFrameTime);
			}
			frameCounter++;
		}

		TaskSystem::TaskManager::Get().WaitAllFinished();
	}

	//-------------------------------------------------------------------------
	
	bool Engine::InitializeModule(Core::IModule* pModule)
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
