#include "TaskManager.h"

#include "Core/Assertion.h"

#include <ranges>
#include <thread>

namespace ZE::TaskSystem
{
	void TaskHandle::Wait() const
	{
		if (m_Payload)
		{
			m_Payload->Wait();
		}
	}
	
	TaskHandle::TaskHandle(std::vector<TaskHandle> dependents)
	{
		m_Payload = std::shared_ptr<TaskHandlePayload>(new TaskHandlePayload(std::move(dependents)));
	}
	
	TaskHandlePayload::TaskHandlePayload(std::vector<TaskHandle> dependents)
		: m_Dependents(std::move(dependents))
	{}
	
	void TaskHandlePayload::Wait() const
	{
		m_Result.wait();
	}

	TaskGroupTaskHandle& TaskGroupTaskHandle::AddDependent(TaskGroupTaskHandle handle)
	{
		m_Task.succeed(handle.m_Task);
		return *this;
	}
	
	TaskGroup::TaskGroup()
	{
		m_TaskGraph = std::make_shared<tf::Taskflow>();
	}

	TaskManager::TaskManager()
	{
		uint32_t numThreads = std::thread::hardware_concurrency();

		InitThreadExecutor(EDedicatedThread::RenderThread, 1);
		numThreads -= 1u;
		InitThreadExecutor(EDedicatedThread::ThreadPool, numThreads);
	}
	
	void TaskManager::InitThreadExecutor(EDedicatedThread thread, uint32_t numThreads)
	{
		m_ThreadExecutorsMap.emplace(thread, new tf::Executor(numThreads));
		auto* pExecutor = m_ThreadExecutorsMap[thread];
		const auto threadInfo = pExecutor->async([&]() -> ThreadInfo
		{
			return { .m_WorkerId = pExecutor->this_worker_id(), .m_ThreadId = std::this_thread::get_id() };
		}).get();
		m_ThreadInfosMap.emplace(thread, threadInfo);
	}
	
	tf::Executor* TaskManager::GetExecutor(EDedicatedThread thread)
	{
		if (auto iter = m_ThreadExecutorsMap.find(thread); iter != m_ThreadExecutorsMap.end())
		{
			return m_ThreadExecutorsMap[thread];
		}
		return nullptr;
	}

	TaskHandle TaskGroup::Run(std::vector<TaskHandle> dependents)
	{
		return TaskManager::Get().RunTaskGroup(*this, std::move(dependents));
	}
	
	TaskManager::~TaskManager()
	{
		WaitAllFinished();
		
		for (auto* pExecutor : std::views::values(m_ThreadExecutorsMap))
		{
			delete pExecutor;
		}
		m_ThreadExecutorsMap.clear();
		m_ThreadInfosMap.clear();
	}
	
	TaskManager& TaskManager::Get()
	{
		static TaskManager sTaskManager;
		return sTaskManager;
	}
	
	TaskHandle TaskManager::RunTaskGroup(TaskGroup& taskGroup, std::vector<TaskHandle> dependents, EDedicatedThread thread)
	{
		auto* pExecutor = GetExecutor(thread);
		if (!pExecutor)
		{
			ZE_LOG_ERROR("Thread executor is missing! Failed to run task group.");
			return {};
		}
		
		if (!dependents.empty())
		{
			TaskHandle handle(std::move(dependents));
			auto result = pExecutor->async([pExecutor, flow = taskGroup.m_TaskGraph, pPayload = handle.m_Payload]
			{
				// wait for all dependents
				// TODO: [Optimization] waste of cpu cycles
				for (const auto& dependent : pPayload->m_Dependents)
				{
					dependent.m_Payload->Wait();
				}
				
				pExecutor->corun(*flow);
			});
			handle.m_Payload->m_Result = result.share();
			return handle;
		}
		
		TaskHandle handle(std::vector<TaskHandle>{});
		handle.m_Payload->m_Result = pExecutor->run(*taskGroup.m_TaskGraph).share();
		return handle;
	}
	
	void TaskManager::WaitUntilFinished(EDedicatedThread thread) const
	{
		if (auto iter = m_ThreadExecutorsMap.find(thread); iter != m_ThreadExecutorsMap.end())
		{
			auto* pExecutor = iter->second;
			pExecutor->wait_for_all();
		}
	}
	
	void TaskManager::WaitAllFinished() const
	{
		for (auto* pExecutor : std::views::values(m_ThreadExecutorsMap))
		{
			pExecutor->wait_for_all();
		}
	}
}
