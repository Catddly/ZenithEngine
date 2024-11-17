#pragma once

#include "Core/Module.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace ZE::Core
{
	class CoreModule : public IModule
	{
	public:

		CoreModule()
			: IModule(ModuleInitializePhase::PreInit, "Core")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		virtual tf::Taskflow& BuildModuleFrameTasks() override;
		virtual void ClearModuleFrameTasks() override;

	private:

		struct MathCalculationData
		{
			glm::mat4 accumMat{ 1.0f };
			glm::vec3 scale{ 0.3f, 0.4f, 0.5f };
		};

		MathCalculationData					m_MathData;
	};
}
