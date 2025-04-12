#pragma once

#include "ModuleDefines.h"

#include <taskflow/taskflow.hpp>

#include <string>
#include <type_traits>

namespace ZE::Engine { class Engine; }

namespace ZE::Core
{
	enum class EModuleInitializePhase : uint8_t
	{
		PreInit = 0,
		Init
	};

	class ENGINE_API IModule
	{
	public:

		IModule(Engine::Engine& engine, EModuleInitializePhase initPhase, const std::string& moduleName = "Unknown")
			: m_Engine(engine), m_TaskFlow(moduleName), m_InitPhase(initPhase), m_ModuleName(moduleName)
		{}
		virtual ~IModule() = default;

		std::string GetModuleName() const { return m_ModuleName; }
		EModuleInitializePhase GetInitializePhase() const { return m_InitPhase; }

		virtual bool InitializeModule() = 0;
		virtual void ShutdownModule() = 0;

		tf::Taskflow& BuildAndGetFrameTasks() { BuildFrameTasks(m_TaskFlow); return m_TaskFlow; }
		tf::Taskflow& GetFrameTasks() { return m_TaskFlow; }
		void ClearFrameTasks() { m_TaskFlow.clear(); }

		/*
		* Build module frame tasks.
		*/
		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) = 0;
			
	public:

		std::reference_wrapper<Engine::Engine>			m_Engine;

	private:

		tf::Taskflow									m_TaskFlow;

		EModuleInitializePhase							m_InitPhase;
		std::string										m_ModuleName;
	};
}
