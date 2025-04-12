#pragma once

#include "Log/Log.h"

#include <assert.h>

// expression inside CHECK() will be eliminated in release version
#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_CHECK(cond) do { if (!(cond)) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#	define ZE_CHECK_LOG(cond, ...) do { if (!(cond)) { ZE_LOG_ERROR(__VA_ARGS__); ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#else
#	define ZE_CHECK(cond)
#	define ZE_CHECK_LOG(cond, ...)
#endif

// expression inside EXEC_CHECK() will be reserved in release version
#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_EXEC_CHECK(cond) do { if (!(cond)) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#	define ZE_EXEC_CHECK_LOG(cond, ...) do { if (!(cond)) { ZE_LOG_ERROR(__VA_ARGS__); ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(false)
#else
#	define ZE_EXEC_CHECK(cond) cond
#	define ZE_EXEC_CHECK_LOG(cond, ...) cond
#endif
