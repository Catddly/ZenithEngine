#pragma once

#include "ModuleDefines.h"

#include "taskflow/taskflow.hpp"

#include <string>

namespace ZE::Core
{
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

		/*
		* Build module frame tasks.
		* @return A taskflow object represent module frame tasks.
		*/
		virtual tf::Taskflow& BuildModuleFrameTasks() = 0;

		virtual void ClearModuleFrameTasks() = 0;
			
	protected:

		tf::Taskflow						m_taskFlow;

	private:

		ModuleInitializePhase				m_InitPhase;
		std::string							m_ModuleName;
	};
}
