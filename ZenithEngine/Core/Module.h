#pragma once

#include "ClassProperty.h"
#include "ModuleDefines.h"

#include <string>
#include <string_view>
#include <type_traits>

namespace ZE::Core
{
	class Engine;
	
	enum class EModuleInitializePhase : uint8_t
	{
		PreInit = 0,
		Init
	};

	class ENGINE_API IModule
	{
	public:

		IModule(Engine& engine, EModuleInitializePhase initPhase, std::string_view moduleName = "Unknown")
			: m_Engine(engine), m_InitPhase(initPhase), m_ModuleName(moduleName)
		{}
		virtual ~IModule() = default;

		ZE_NON_COPYABLE_AND_NON_MOVABLE_CLASS(IModule);
		
		std::string GetModuleName() const { return m_ModuleName; }
		EModuleInitializePhase GetInitializePhase() const { return m_InitPhase; }

		Engine& GetEngine() { return m_Engine; }
		const Engine& GetEngine() const { return m_Engine; }

		virtual bool InitializeModule() = 0;
		virtual void ShutdownModule() = 0;
	
	public:

		std::reference_wrapper<Engine>					m_Engine;

	private:

		EModuleInitializePhase							m_InitPhase;
		std::string										m_ModuleName;
	};
}
