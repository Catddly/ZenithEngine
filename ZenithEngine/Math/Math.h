#pragma once

#include <type_traits>

namespace ZE::Math
{
	template <typename T>
	concept IsIntegral = std::is_integral_v<T>;

	template <IsIntegral T>
	T AlignTo(T value, T alignment)
	{
		T remain = value % alignment;
		return remain ? value + (alignment - remain) : value;
	}
}
