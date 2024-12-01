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
			: m_InitPhase(initPhase), m_ModuleName(moduleName), m_TaskFlow(moduleName)
		{}
		virtual ~IModule() = default;

		inline std::string GetModuleName() const { return m_ModuleName; }
		inline ModuleInitializePhase GetInitializePhase() const { return m_InitPhase; }

		virtual bool InitializeModule() = 0;
		virtual void ShutdownModule() = 0;

		tf::Taskflow& BuildAndGetFrameTasks() { BuildFrameTasks(m_TaskFlow); return m_TaskFlow; }
		tf::Taskflow& GetFrameTasks() { return m_TaskFlow; }
		void ClearFrameTasks() { m_TaskFlow.clear(); }

		/*
		* Build module frame tasks.
		*/
		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) = 0;
			
	private:

		tf::Taskflow						m_TaskFlow;

		ModuleInitializePhase				m_InitPhase;
		std::string							m_ModuleName;
	};
}
