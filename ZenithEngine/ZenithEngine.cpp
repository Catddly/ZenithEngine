#include "ZenithEngine.h"

#include <assert.h>

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
		assert(m_Engine.PreInitialize());
		assert(m_Engine.Initialize());
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