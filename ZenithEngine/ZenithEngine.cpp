#include "ZenithEngine.h"

#include "Core/Assertion.h" 

namespace ZE
{
	void RunZenithEngine(Engine::Engine& engine)
	{
		engine.PreInitialize();
		engine.Initialize();

		engine.Run();

		engine.Shutdown();
		engine.PostShutdown();
	}

	//-------------------------------------------------------------------------

	RunEngineScoped::RunEngineScoped()
	{
		ZE_EXEC_CHECK(m_Engine.PreInitialize());
		ZE_EXEC_CHECK(m_Engine.Initialize());
	}

	RunEngineScoped::~RunEngineScoped()
	{
		m_Engine.Shutdown();
		m_Engine.PostShutdown();
	}

	void RunEngineScoped::Run()
	{
		m_Engine.Run();
	}
}