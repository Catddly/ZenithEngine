#pragma once

#include "ModuleDefines.h"

#include <string>
#include <string_view>
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

		IModule(Engine::Engine& engine, EModuleInitializePhase initPhase, std::string_view moduleName = "Unknown")
			: m_Engine(engine), m_InitPhase(initPhase), m_ModuleName(moduleName)
		{}
		virtual ~IModule() = default;

		std::string GetModuleName() const { return m_ModuleName; }
		EModuleInitializePhase GetInitializePhase() const { return m_InitPhase; }

		Engine::Engine& GetEngine() { return m_Engine; }
		const Engine::Engine& GetEngine() const { return m_Engine; }

		virtual bool InitializeModule() = 0;
		virtual void ShutdownModule() = 0;
	
	public:

		std::reference_wrapper<Engine::Engine>			m_Engine;

	private:

		EModuleInitializePhase							m_InitPhase;
		std::string										m_ModuleName;
	};
}
