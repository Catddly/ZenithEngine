#pragma once

#include "ModuleDefines.h"
#include "Core/Engine.h"

namespace ZE
{
	ENGINE_API void RunZenithEngine(Core::Engine& engine);

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

		Core::Engine				m_Engine;
	};
}