#pragma once

#include "Module.h"
#include "Core/ModuleManager.h"

class LogModule;

class ENGINE_API ZenithEngine
{
public:

	virtual ~ZenithEngine() = default;

	virtual bool PreInitialize();
	virtual bool Initialize();
	virtual void Shutdown();
	virtual void PostShutdown();

	virtual void Run();

private:

	LogModule*					m_pLogModule = nullptr;

	ModuleManager				m_ModuleManager;
	bool						m_bIsPreInitialized = false;
	bool						m_bIsInitialized = false;
	bool						m_RequestExit = false;
};