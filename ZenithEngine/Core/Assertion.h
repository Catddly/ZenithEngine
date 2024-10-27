#pragma once

#include "Log/Log.h"
#include <assert.h>

#if ENABLE_ZENITH_RUNTIME_CHECK
#	define ZE_CHECK(cond) do { if (!cond) { ZE_FLUSH_LOG(); __debugbreak(); assert(false); } } while(0);
#else
#	define ZE_CHECK(cond)
#endif