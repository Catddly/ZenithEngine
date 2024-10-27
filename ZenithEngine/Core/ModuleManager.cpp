#include "ModuleManager.h"
#include "Core/Assertion.h"
#include "Core/CoreModule.h"
#include "Log/Log.h"
#include <ranges>
#include <algorithm>
#include <format>

void ModuleManager::RegisterModule(IModule* module)
{
	ZE_CHECK(module);

	if (module->GetInitializePhase() == ModuleInitializePhase::PreInit)
	{
		m_PreInitSystemModules.push_back(module);
	}
	else if (module->GetInitializePhase() == ModuleInitializePhase::Init)
	{
		m_InitSystemModules.push_back(module);
	}
}

void ModuleManager::UnregisterModule(IModule* module)
{
	std::erase_if(m_PreInitSystemModules, [module](const IModule* innerModule)
	{
		return module == innerModule;
	});
	std::erase_if(m_InitSystemModules, [module](const IModule* innerModule)
	{
		return module == innerModule;
	});
}

void ModuleManager::UnregisterModule(const std::string& moduleName)
{
	std::erase_if(m_PreInitSystemModules, [&moduleName](const IModule* module)
	{
		return module->GetModuleName() == moduleName;
	});
	std::erase_if(m_InitSystemModules, [&moduleName](const IModule* module)
	{
		return module->GetModuleName() == moduleName;
	});
}

ModuleManager::InitializeResult ModuleManager::PreInitializeModules()
{
	InitializeResult initResult;

	// initialize with register order
	for (auto& module : m_PreInitSystemModules)
	{
		bool result = module->InitializeModule();
		
		if (!result)
		{
			// halt immediately
			initResult.m_bAllSuccess = false;
			initResult.m_FailedLog = std::format("Module pre-initialization failed:\n\tModule [{}] failed to initialize.", module->GetModuleName());
			break;
		}

		ZE_LOG_INFO("Module [{}] initialized.", module->GetModuleName());
	}

	return initResult;
}

ModuleManager::InitializeResult ModuleManager::InitializeModules()
{
	InitializeResult initResult;

	// initialize with register order
	for (auto& module : m_InitSystemModules)
	{
		bool result = module->InitializeModule();

		if (!result)
		{
			// halt immediately
			initResult.m_bAllSuccess = false;
			initResult.m_FailedLog = std::format("Module initialization failed:\n\tModule [{}] failed to initialize.", module->GetModuleName());
			break;
		}
	
		ZE_LOG_INFO("Module [{}] initialized.", module->GetModuleName());
	}

	return initResult;
}

void ModuleManager::ShutdownModules()
{
	// shutdown with reverse register order
	for (auto& module : std::views::reverse(m_PreInitSystemModules))
	{
		module->ShutdownModule();
		ZE_LOG_INFO("Module [{}] shutdown.", module->GetModuleName());
	}
}

void ModuleManager::PostShutdownModules()
{
	// shutdown with reverse register order
	for (auto& module : m_PreInitSystemModules)
	{
		module->ShutdownModule();
		ZE_LOG_INFO("Module [{}] shutdown.", module->GetModuleName());
	}
}
