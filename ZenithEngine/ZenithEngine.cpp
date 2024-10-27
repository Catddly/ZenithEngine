#include "ZenithEngine.h"

void RunZenithEngine(ZenithEngine& engine)
{
	engine.PreInitialize();
	engine.Initialize();

	engine.Run();
	
	engine.Shutdown();
	engine.PostShutdown();
}
