#pragma once

#include "Core/ModuleInterface.h"

class CoreModule : public IModule
{
public:

	virtual bool InitializeModule() override;
	virtual void ShutdownModule() override;

private:

};