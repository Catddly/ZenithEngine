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

	tf::Taskflow& CoreModule::BuildModuleFrameTasks()
	{
		tf::Task firstTask = m_taskFlow.emplace([]()
		{
			ZE_LOG_INFO("Init from first task!");
		});

		tf::Task secondTask = m_taskFlow.emplace([]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			ZE_LOG_INFO("Init from second tasks!");
		});

		tf::Task mathTask = m_taskFlow.placeholder();
		mathTask.data(&m_MathData).work([mathTask]() {
			MathCalculationData* pData = static_cast<MathCalculationData*>(mathTask.data());
			ZE_CHECK(pData);

			ZE_LOG_INFO("Calculating Math...");

			pData->accumMat = glm::scale(pData->accumMat, pData->scale);
		});

		firstTask.precede(secondTask, mathTask);

		return m_taskFlow;
	}

	void CoreModule::ClearModuleFrameTasks()
	{
		m_taskFlow.clear();
	}
}
