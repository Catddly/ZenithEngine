#include "Core.h"

#include "Core/Assertion.h"
#include "Math/MathFormat.h"
#include "Log/Log.h"
#include "Platform/Displayable.h"
#include "Platform/Window.h"

#include <thread>
#include <chrono>

namespace ZE::Core
{
    bool CoreModule::InitializeModule()
    {
		m_pDisplayable = new Platform::Displayble;
		if (!m_pDisplayable->Initialize())
		{
			m_pDisplayable->Shutdown();
			return false;
		}

        return true;
    }

    void CoreModule::ShutdownModule()
    {
		ZE_LOG_INFO("Output math result:\n{}", m_MathData.accumMat);
	
		if (m_pDisplayable)
		{
			m_pDisplayable->Shutdown();
			delete m_pDisplayable;
		}
    }

	//-------------------------------------------------------------------------

	void CoreModule::BuildFrameTasks(tf::Taskflow& taskFlow)
	{
		tf::Task firstTask = taskFlow.emplace([]()
		{
			ZE_LOG_INFO("Init from first task!");
		});

		tf::Task secondTask = taskFlow.emplace([]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			ZE_LOG_INFO("Init from second tasks!");
		});

		tf::Task mathTask = taskFlow.placeholder();
		mathTask.data(&m_MathData).work([mathTask]() {
			MathCalculationData* pData = static_cast<MathCalculationData*>(mathTask.data());
			ZE_CHECK(pData);

			ZE_LOG_INFO("Calculating Math...");

			pData->accumMat = glm::scale(pData->accumMat, pData->scale);
		});

		firstTask.precede(secondTask, mathTask);

		//-------------------------------------------------------------------------
	
		m_pDisplayable->ProcessPlatformEvents();
	}
}
