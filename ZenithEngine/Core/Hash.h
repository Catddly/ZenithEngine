#pragma once

#include <cstdint>
#include <functional>

namespace ZE::Core
{
	template <typename T>
	concept StdHashable = requires
	{
		requires std::hash<T>{}();
	};
	
	template <typename StdHashable>
	uint64_t Hash(const StdHashable& value)
	{
		return std::hash<StdHashable>{}(value);
	}

	template <typename StdHashable>
	uint64_t Hash(const uint64_t seed, const StdHashable& value)
	{
		return seed ^ std::hash<StdHashable>{}(value);
	}
}
