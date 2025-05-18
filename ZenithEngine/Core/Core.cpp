#include "Core.h"

#include "Core/Assertion.h"
#include "Math/MathFormat.h"
#include "Log/Log.h"
#include "Platform/Displayable.h"

#include "FileSystem.h"
#include "Asset/Asset.h"

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

		FileSystem::Mount();

        return true;
    }

    void CoreModule::ShutdownModule()
    {
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
