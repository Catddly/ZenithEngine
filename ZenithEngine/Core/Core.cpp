#include "Core.h"

#include "Core/Assertion.h"
#include "Log/Log.h"
#include "Math/MathFormat.h"

#include <thread>
#include <chrono>

namespace ZE::Core
{
    bool CoreModule::InitializeModule()
    {
        return true;
    }

    void CoreModule::ShutdownModule()
    {
		ZE_LOG_INFO("Output math result:\n{}", m_MathData.accumMat);
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
	}
}
