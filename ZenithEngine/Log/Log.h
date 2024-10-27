#pragma once

#include "Module.h"
#include "Core/ModuleInterface.h"
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include <memory>

class LogModule : public IModule
{
public:

	LogModule()
		: IModule(ModuleInitializePhase::PreInit, "Log")
	{}

	bool InitializeModule() override;
	void ShutdownModule() override;

private:

	std::shared_ptr<spdlog::logger>				m_Logger;
};

#define ZE_LOG_VERBOSE(...) spdlog::trace(__VA_ARGS__)
#define ZE_LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define ZE_LOG_WARNING(...) spdlog::warn(__VA_ARGS__)
#define ZE_LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define ZE_LOG_FATAL(...) spdlog::critical(__VA_ARGS__)

// TODO: change level to engine log level
#define ZE_FLUSH_LOG() spdlog::flush_on(spdlog::level::info)