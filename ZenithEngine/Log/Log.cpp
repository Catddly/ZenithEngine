#include "Log.h"

#include "Core/Assertion.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <chrono>

namespace ZE::Log
{
	bool LogModule::InitializeModule()
	{
		ZE_CHECK(!m_Logger);

		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		// %^...%$ will print text in color
		consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/ZenithLog.txt", true);
		fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

		auto sinks = std::initializer_list<spdlog::sink_ptr>{ consoleSink, fileSink };
		m_Logger = std::make_shared<spdlog::logger>("ZenithLogger", std::move(sinks));

		spdlog::set_default_logger(m_Logger);
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_every(std::chrono::seconds(5));

		return true;
	}

	void LogModule::ShutdownModule()
	{
		ZE_CHECK(m_Logger);

		m_Logger.reset();
	}

	void LogModule::BuildFrameTasks(tf::Taskflow& taskFlow)
{
		auto [condSwitch, cond, trueTask, falseTask] = taskFlow.emplace(
			[this]() { m_TestBranch = !m_TestBranch; },
			[this]() { return m_TestBranch; },
			&TestStaticLoggerTask,
			[this]() { TestLoggerTask(); }
		);

		condSwitch.precede(cond);
		cond.precede(falseTask, trueTask);
	}

	void LogModule::TestStaticLoggerTask()
	{
		// std::this_thread::sleep_for(std::chrono::seconds(1));
		// ZE_LOG_INFO("Test static logger task!");
	}

	void LogModule::TestLoggerTask()
	{
		// std::this_thread::sleep_for(std::chrono::milliseconds(30));
		// ZE_LOG_INFO("Test logger task!");
	}

}
