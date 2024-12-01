#pragma once

#include "ModuleDefines.h"
#include "Core/Module.h"

#include "spdlog/spdlog.h"
#include "spdlog/logger.h"

#include <memory>

namespace ZE::Log
{
	class LogModule : public Core::IModule
	{
	public:

		LogModule()
			: Core::IModule(Core::ModuleInitializePhase::PreInit, "Log")
		{}

		bool InitializeModule() override;
		void ShutdownModule() override;

		virtual void BuildFrameTasks(tf::Taskflow& taskFlow) override;

	private:

		static void TestStaticLoggerTask();
		void TestLoggerTask();

	private:

		std::shared_ptr<spdlog::logger>				m_Logger;
	
		bool										m_TestBranch = false;
	};
}

#define ZE_LOG_VERBOSE(...) spdlog::trace(__VA_ARGS__)
#define ZE_LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define ZE_LOG_WARNING(...) spdlog::warn(__VA_ARGS__)
#define ZE_LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define ZE_LOG_FATAL(...) spdlog::critical(__VA_ARGS__)

// TODO: change level to engine log level
#define ZE_FLUSH_LOG() spdlog::flush_on(spdlog::level::info)
