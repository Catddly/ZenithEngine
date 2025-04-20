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
}
