#pragma once

#include "ModuleDefines.h"
#include "Engine/Engine.h"

namespace ZE
{
	ENGINE_API void RunZenithEngine(Engine::ZenithEngine& engine);

	class ENGINE_API RunEngineScoped
	{
	public:

		RunEngineScoped();
		~RunEngineScoped();

		void Run();

	private:

		Engine::ZenithEngine				m_Engine;
	};
}