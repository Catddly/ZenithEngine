#pragma once

#include "Core/Assertion.h"
#include "Core/Reflection.h"

#include <taskflow/taskflow.hpp>

#include <string>
#include <expected>
#include <unordered_map>
#include <type_traits>

namespace ZE::TaskSystem
{
	// TODO:
	// 1. custom return type and input parameters task
	// 2. task graph debug
	// 3. propagation of thread panic and task failed error
	
	enum class ETaskErrorType : uint8_t
	{
		FailedToExecute = 0,
		ThreadPanic
	};
	
	struct TaskExecutionError
	{
		ETaskErrorType				m_ErrorType = ETaskErrorType::FailedToExecute;
		std::string					m_ErrorMessage;
	};

	enum class EDedicatedThread : uint8_t
	{
		RenderThread = 0,
		ThreadPool = 100,
	};

	using TaskReturnType = std::expected<bool, TaskExecutionError>;

	class TaskHandlePayload;
	class TaskHandle
	{
		friend class TaskManager;
		
	public:

		TaskHandle() = default;

		void Wait() const;
		
	private:

		explicit TaskHandle(std::vector<TaskHandle> dependents);
		
	private:

		std::shared_ptr<TaskHandlePayload>			m_Payload = nullptr;
	};
	
	class TaskHandlePayload
	{
		friend class TaskHandle;
		friend class TaskManager;

	private:

		explicit TaskHandlePayload(std::vector<TaskHandle> dependents);

		void Wait() const;
		
	private:

		// Very ugly!!
		std::shared_future<void>					m_Result;
		// std::variant<std::shared_future<>>			m_Result;
		std::vector<TaskHandle>						m_Dependents;
	};

	class TaskGroupTaskHandle
	{
		friend class TaskGroup;
			
	public:

		TaskGroupTaskHandle& AddDependent(TaskGroupTaskHandle handle);
			
	private:

		tf::Task 								m_Task;
	};
	
	class TaskGroup
	{
		friend class TaskManager;
		
	public:

		TaskGroup();

		template <typename Func, typename RetType = std::invoke_result_t<Func>>
		TaskGroupTaskHandle AddTask(Func&& func);

		TaskHandle Run(std::vector<TaskHandle> dependents = {});

	private:
		
		std::shared_ptr<tf::Taskflow>			m_TaskGraph;
	};
	
	class TaskManager final
	{
	public:

		~TaskManager();

		TaskManager(const TaskManager&) = delete;
		TaskManager& operator=(const TaskManager&) = delete;
		TaskManager(TaskManager&&) = delete;
		TaskManager& operator=(TaskManager&&) = delete;
		
		static TaskManager& Get();

		template <typename Func, typename RetType = std::invoke_result_t<Func>>
		TaskHandle RunTask(Func&& func, std::vector<TaskHandle> dependents = {}, EDedicatedThread thread = EDedicatedThread::ThreadPool);
		
		TaskHandle RunTaskGroup(TaskGroup& taskGroup, std::vector<TaskHandle> dependents = {}, EDedicatedThread thread = EDedicatedThread::ThreadPool);

		// Wait until all tasks in the thread is finished
		void WaitUntilFinished(EDedicatedThread thread) const;
		void WaitAllFinished() const;

	private:
		
		TaskManager();

		void InitThreadExecutor(EDedicatedThread thread, uint32_t numThreads);
		tf::Executor* GetExecutor(EDedicatedThread thread);

		struct ThreadInfo
		{
			int						m_WorkerId = -1;
			std::thread::id			m_ThreadId;
		};
		
	private:

		std::unordered_map<EDedicatedThread, tf::Executor*>			m_ThreadExecutorsMap;
		std::unordered_map<EDedicatedThread, ThreadInfo>			m_ThreadInfosMap;
	};
	
	template <typename Func, typename RetType>
	TaskGroupTaskHandle TaskGroup::AddTask(Func&& func)
	{
		ZE_CHECK(m_TaskGraph);
		static_assert(std::is_same_v<RetType, void>);
		
		TaskGroupTaskHandle handle;
		handle.m_Task = m_TaskGraph->emplace(std::forward<Func>(func));
		return handle;
	}
	
	template <typename Func, typename RetType>
	TaskHandle TaskManager::RunTask(Func&& func, std::vector<TaskHandle> dependents, EDedicatedThread thread)
	{
		static_assert(std::is_same_v<RetType, void>);

		auto* pExecutor = GetExecutor(thread);
		if (!pExecutor)
		{
			ZE_LOG_ERROR("Thread executor is missing! Failed to run task.");
			return {};
		}
		
		if (!dependents.empty())
		{
			TaskHandle handle(std::move(dependents));
			std::future<void> result = pExecutor->async([this, localFunc = std::forward<Func>(func), pPayload = handle.m_Payload]
			{
				// wait for all dependents
				// TODO: [Optimization] waste of cpu cycles
				for (const auto& dependent : pPayload->m_Dependents)
				{
					dependent.m_Payload->Wait();
				}

				// execute self task
				localFunc();
			});
			handle.m_Payload->m_Result = result.share();
			return handle;
		}
		
		TaskHandle handle(std::vector<TaskHandle>{});
		std::future<void> result = pExecutor->async([localFunc = std::forward<Func>(func)]
		{
			localFunc();
		});
		handle.m_Payload->m_Result = result.share();
		return handle;
	}
	
	// Task to wrap any member function, static function or lambda into a lifetime object
	// Only Accept a void() function for now.
	// class Task
	// {
	// 	friend class TaskManager;
	//
	// private:
	// 	
	// 	struct FuncVTable
	// 	{
	// 		void(*m_TaskFuncRedirector)(void* pFuncStorage);
	// 		void(*m_ReleaseTaskMemory)(void* pFuncStorage);
	// 	};
	// 	
	// 	template <typename Func>
	// 	static constexpr FuncVTable skVtableEntry =
	// 	{
	// 		.m_TaskFuncRedirector = +[](void* pFuncStorage){ (*static_cast<Func*>(pFuncStorage))(); },
	// 		.m_ReleaseTaskMemory = +[](void* pFuncStorage){ auto* pStorage = static_cast<Func*>(pFuncStorage); delete pStorage; }
	// 	};
	// 	
	// public:
	//
	// 	template <typename Func>
	// 	explicit Task(Func func)
	// 		: m_VTable(&skVtableEntry<Func>)
	// 	{
	// 		auto* pFunc = new Func(std::move(func));
	// 		m_FuncStorage = static_cast<void*>(pFunc);
	// 	}
	//
	// 	Task(const Task&) = delete;
	// 	Task& operator=(const Task&) = delete;
	// 	Task(Task&&) = default;
	// 	Task& operator=(Task&&) = default;
	//
	// 	~Task()
	// 	{
	// 		if (m_VTable)
	// 		{
	// 			m_VTable->m_ReleaseTaskMemory(m_FuncStorage);
	// 		}
	// 	}
	//
	// private:
	//
	// 	void Execute() const;
	// 	
	// private:
	//
	// 	const FuncVTable*								m_VTable = nullptr;
	// 	void*											m_FuncStorage = nullptr;
	// 	
	// 	bool											m_HadDispatched = false;
	// };
}
