#pragma once

#include "Log/Log.h"
#include <assert.h>

#if ZENITH_ENABLE_RUNTIME_CHECK
#	define ZE_CHECK(cond) do { if (!(cond)) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(0);
#	define ZE_CHECK_LOG(cond, ...) do { if (!(cond)) { ZE_LOG_ERROR(__VA_ARGS__); ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(0);
#else
#	define ZE_CHECK(cond)
#	define ZE_CHECK_LOG(cond)
#endif