#pragma once

#include "Log/Log.h"

#include <cstring>

#define VulkanZeroStruct(TypeName, Name) TypeName Name; memset(&Name, 0, sizeof(TypeName))

#define VulkanCheckSucceed(func) do { if ( ((VkResult) func) != VK_SUCCESS ) { ZE_LOG_FATAL("Vulkan failure!"); } } while(0)
