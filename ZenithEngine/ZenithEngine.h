#pragma once

#include "ModuleDefines.h"
#include "Engine/Engine.h"

namespace ZE
{
	ENGINE_API void RunZenithEngine(Engine::Engine& engine);

	class ENGINE_API RunEngineScoped
	{
	public:

		RunEngineScoped();
		~RunEngineScoped();
		
		RunEngineScoped(const RunEngineScoped&) = delete;
		RunEngineScoped& operator=(const RunEngineScoped&) = delete;
		RunEngineScoped(RunEngineScoped&&) = delete;
		RunEngineScoped& operator=(RunEngineScoped&&) = delete;

		void Run();

	private:

		Engine::Engine				m_Engine;
	};
}