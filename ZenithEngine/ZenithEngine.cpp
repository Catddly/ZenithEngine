#include "ZenithEngine.h"

namespace ZE
{
	void RunZenithEngine(Engine::ZenithEngine& engine)
	{
		engine.PreInitialize();
		engine.Initialize();

		engine.Run();

		engine.Shutdown();
		engine.PostShutdown();
	}
}