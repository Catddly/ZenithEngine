#include "Engine.h"
#include "Core/Assertion.h"
#include "Log/Log.h"

bool ZenithEngine::PreInitialize()
{
	m_pLogModule = new LogModule;

	m_ModuleManager.RegisterModule(m_pLogModule);

	//-------------------------------------------------------------------------
	
	ZE_CHECK(!m_bIsPreInitialized);

	auto result = m_ModuleManager.PreInitializeModules();
	if (!result.m_bAllSuccess)
	{
		ZE_LOG_FATAL("Zenith engine failed to pre-initialized with:\n\t{}", result.m_FailedLog);
		return false;
	}

	m_bIsPreInitialized = true;
	return true;
}

bool ZenithEngine::Initialize()
{
	ZE_CHECK(!m_bIsInitialized);

	auto result = m_ModuleManager.InitializeModules();
	if (!result.m_bAllSuccess)
	{
		ZE_LOG_FATAL("Zenith engine failed to pre-initialized with:\n\t{}", result.m_FailedLog);
		return false;
	}

	m_bIsInitialized = true;
	return true;
}

void ZenithEngine::Shutdown()
{
	ZE_CHECK(m_bIsInitialized);

	m_ModuleManager.ShutdownModules();
}

void ZenithEngine::PostShutdown()
{
	ZE_CHECK(m_bIsPreInitialized);
	m_ModuleManager.PostShutdownModules();

	//-------------------------------------------------------------------------

	delete m_pLogModule;
}

void ZenithEngine::Run()
{
	ZE_CHECK(m_bIsPreInitialized);
	ZE_CHECK(m_bIsInitialized);

	while (!m_RequestExit)
	{
		//ZE_LOG_INFO("Zenith engine is running!");
	}
}
