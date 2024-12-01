#pragma once

#include "Core/Module.h"

namespace ZE::Render
{
	class RenderModule : public Core::IModule
	{
	public:

		RenderModule()
			: Core::IModule(Core::ModuleInitializePhase::Init, "Render")
		{}

		virtual bool InitializeModule() override;
		virtual void ShutdownModule() override;

		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) override;
	
	private:

	};
}