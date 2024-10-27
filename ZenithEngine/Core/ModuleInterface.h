#pragma once

#include "Module.h"
#include <string>

enum class ModuleInitializePhase
{
	PreInit = 0,
	Init
};

class ENGINE_API IModule
{
public:
	IModule(ModuleInitializePhase initPhase, const std::string& moduleName = "Unknown")
		: m_InitPhase(initPhase), m_ModuleName(moduleName)
	{}
	virtual ~IModule() = default;

	inline std::string GetModuleName() const { return m_ModuleName; }
	inline ModuleInitializePhase GetInitializePhase() const { return m_InitPhase; }

	virtual bool InitializeModule() = 0;
	virtual void ShutdownModule() = 0;

private:

	ModuleInitializePhase				m_InitPhase;
	std::string							m_ModuleName;
};