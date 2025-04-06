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

		void Run();

	private:

		Engine::Engine				m_Engine;
	};
}