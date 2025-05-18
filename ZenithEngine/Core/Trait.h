#pragma once

#include <type_traits>

namespace ZE::Core
{
	template <typename T>
	concept CopyableAndMovable = requires
	{
		requires std::is_copy_constructible_v<T>
			&& std::is_copy_assignable_v<T>
			&& std::is_move_constructible_v<T>
			&& std::is_move_assignable_v<T>;
	};
	
	template <typename T>
	concept CopyableButNonMovable = requires
	{
		requires std::is_copy_constructible_v<T>
			&& std::is_copy_assignable_v<T>
			&& !std::is_move_constructible_v<T>
			&& !std::is_move_assignable_v<T>;
	};

	template <typename T>
	concept NonCopyableAndNonMovable = requires
	{
		requires !std::is_copy_constructible_v<T>
			&& !std::is_copy_assignable_v<T>
			&& !std::is_move_constructible_v<T>
			&& !std::is_move_assignable_v<T>;
	};
}
