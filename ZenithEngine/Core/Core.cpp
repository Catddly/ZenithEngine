#include "Core.h"

#include "Core/Assertion.h"
#include "Math/MathFormat.h"
#include "Log/Log.h"
#include "Platform/Displayable.h"

#include <chrono>

namespace ZE::Core
{
    bool CoreModule::InitializeModule()
    {
		m_DisplayDevice = new Platform::Displayable;
		if (!m_DisplayDevice->Initialize())
		{
			m_DisplayDevice->Shutdown();
			return false;
		}

        return true;
    }

    void CoreModule::ShutdownModule()
    {
		ZE_LOG_INFO("Output math result:\n{}", m_MathData.accumMat);
	
		if (m_DisplayDevice)
		{
			m_DisplayDevice->Shutdown();
			delete m_DisplayDevice;
		}
    }

	//-------------------------------------------------------------------------
	
	void CoreModule::ProcessPlatformEvents()
    {
    	m_DisplayDevice->ProcessPlatformEvents();
    }
}
