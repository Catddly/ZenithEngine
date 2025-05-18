#pragma once

#include "Log/Log.h"

#if ZENITH_ENABLE_RUNTIME_CHECK
#	include <assert.h>
#	include <utility>
#	include <stacktrace>
#	include <source_location>
#endif

#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_LOG_STACKTRACE() do { std::stacktrace st = std::stacktrace::current(); \
		for (const auto& frame : st) { ZE_LOG_FATAL("\t{}", std::to_string(frame)); } } while(false)

#	define ZE_LOG_CRASH_POINT() do { ZE_LOG_FATAL("Fatal error occur in {} [Line {}] [Function: {}]", \
	std::source_location::current().file_name(), \
	std::source_location::current().line(), \
	std::source_location::current().function_name()); } while (false)
#else
#	define ZE_LOG_STACKTRACE()

#	define ZE_LOG_CRASH_POINT()
#endif

// expression inside ASSERT() will be eliminated in release version
#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_ASSERT(cond) do { if (!(cond)) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#	define ZE_ASSERT_LOG(cond, ...) do { if (!(cond)) { ZE_LOG_FATAL(__VA_ARGS__); ZE_LOG_CRASH_POINT(); ZE_LOG_STACKTRACE(); ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#else
#	define ZE_ASSERT(cond)
#	define ZE_ASSERT_LOG(cond, ...)
#endif

// expression inside EXEC_CHECK() will be reserved in release version
#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_EXEC_ASSERT(cond) do { if (!(cond)) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#	define ZE_EXEC_ASSERT_LOG(cond, ...) do { if (!(cond)) { ZE_LOG_FATAL(__VA_ARGS__); ZE_LOG_CRASH_POINT(); ZE_LOG_STACKTRACE(); ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#else
#	define ZE_EXEC_ASSERT(cond) cond
#	define ZE_EXEC_ASSERT_LOG(cond, ...) cond
#endif

#if ZENITH_ENABLE_RUNTIME_CHECK
#define ZE_UNREACHABLE() do { ZE_LOG_CRASH_POINT(); ZE_LOG_STACKTRACE(); std::unreachable(); } while(false)
#else
#define ZE_UNREACHABLE()
#endif

#if ZENITH_ENABLE_RUNTIME_CHECK
#define ZE_UNIMPLEMENTED() do { ZE_LOG_CRASH_POINT(); ZE_LOG_STACKTRACE(); std::unreachable(); } while(false)
#else
#define ZE_UNIMPLEMENTED()
#endif
