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

	void CoreModule::BuildFrameTasks(tf::Taskflow& taskFlow)
	{
		// tf::Task processPlatformEvent = taskFlow.emplace([this]()
		// {
		// });
		//
		// tf::Task secondTask = taskFlow.emplace([]()
		// {
		// 	// std::this_thread::sleep_for(std::chrono::seconds(1));
		// 	// ZE_LOG_INFO("Init from second tasks!");
		// });
		//
		// tf::Task mathTask = taskFlow.placeholder();
		// mathTask.data(&m_MathData).work([mathTask]() {
		// 	MathCalculationData* pData = static_cast<MathCalculationData*>(mathTask.data());
		// 	ZE_CHECK(pData);
		//
		// 	// ZE_LOG_INFO("Calculating Math...");
		//
		// 	pData->accumMat = glm::scale(pData->accumMat, pData->scale);
		// });

		// processPlatformEvent.precede(secondTask, mathTask);
	}
	
	void CoreModule::ProcessPlatformEvents()
    {
    	m_DisplayDevice->ProcessPlatformEvents();
    }
}
