#pragma once

#include "Module.h"
#include <vector>
#include <string>

class IModule;

class ModuleManager
{
public:

	void RegisterModule(IModule* module);

	void UnregisterModule(IModule* module);
	void UnregisterModule(const std::string& moduleName);

	struct InitializeResult
	{
		std::string				m_FailedLog;
		bool					m_bAllSuccess = true;
	};

	InitializeResult PreInitializeModules();
	InitializeResult InitializeModules();
	
	void ShutdownModules();
	void PostShutdownModules();

private:

	std::vector<IModule*>				m_PreInitSystemModules;
	std::vector<IModule*>				m_InitSystemModules;
};
