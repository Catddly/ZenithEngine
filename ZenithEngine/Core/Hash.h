#pragma once

#include <cstdint>
#include <functional>

namespace ZE::Core
{
	template <typename T>
	concept Hashable = requires
	{
		requires std::hash<T>{}();
	};
	
	template <typename Hashable>
	uint64_t Hash(const Hashable& value)
	{
		return std::hash<Hashable>{}(value);
	}

	template <typename Hashable>
	uint64_t Hash(const uint64_t seed, const Hashable& value)
	{
		return seed ^ std::hash<Hashable>{}(value);
	}
}
