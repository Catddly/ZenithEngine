#pragma once

#include "refl.hpp"

#include <type_traits>

namespace ZE::Core
{
	template <typename T>
	concept IsDefaultConstrctible = std::is_default_constructible_v<T>;

	template <typename T>
	concept IsTriviallyDestructible = std::is_trivially_destructible_v<T>;

	template <size_t N>
	class CompileTimeStr
	{
	public:

		static constexpr size_t npos = static_cast<size_t>(-1);
		static constexpr size_t size = N;

		char					m_Data[N + 1];

		constexpr CompileTimeStr() noexcept
			: m_Data{}
		{
		}

		constexpr CompileTimeStr(const CompileTimeStr<N>& other) noexcept
			: CompileTimeStr(other, std::make_index_sequence<N>())
		{
		}

		constexpr CompileTimeStr(const char(&data)[N + 1]) noexcept
			: CompileTimeStr(data, std::make_index_sequence<N>())
		{
		}

		explicit constexpr operator const char* () const noexcept
		{
			return m_Data;
		}

		constexpr const char* Str() const noexcept
		{
			return m_Data;
		}

		constexpr std::string String() const noexcept
		{
			return m_Data;
		}

		template <size_t Pos, size_t Count = npos>
		constexpr auto SubStr() const noexcept
		{
			static_assert(Pos <= N);
			constexpr size_t newSize = std::min(Count, N - Pos);

			char buf[newSize + 1]{};
			for (size_t i = 0; i < newSize; i++)
			{
				buf[i] = m_Data[Pos + i];
			}

			return CompileTimeStr<newSize>(buf);
		}

		constexpr auto ToUpper() const noexcept
		{
			char buf[N + 1]{};
			for (size_t i = 0; i < N + 1; i++)
			{
				char upper = static_cast<uint8_t>(m_Data[i]) >= 'a' && static_cast<uint8_t>(m_Data[i]) <= 'z' ? static_cast<char>(static_cast<uint8_t>(m_Data[i]) - 'a' + 'A') : m_Data[i];
				buf[i] = upper;
			}

			return CompileTimeStr<N>(buf);
		}

	private:

		template <size_t... Idx>
		constexpr CompileTimeStr(const CompileTimeStr<N>& other, std::index_sequence<Idx...>) noexcept
			: m_Data{ other.m_Data[Idx]... }
		{
		}

		template <size_t... Idx>
		constexpr CompileTimeStr(const char(&data)[sizeof...(Idx) + 1], std::index_sequence<Idx...>) noexcept
			: m_Data{ data[Idx]... }
		{
		}
	};

	constexpr CompileTimeStr<0> ConstructCompileTimeStr() noexcept
	{
		return {};
	}

	template <size_t N>
	constexpr CompileTimeStr<N - 1> ConstructCompileTimeStr(const char(&str)[N]) noexcept
	{
		return str;
	}

	constexpr CompileTimeStr<1> ConstructCompileTimeStr(char c) noexcept
	{
		const char str[2]{ c, '\0' };
		return str;
	}

	template <typename T>
	constexpr auto GetTypeName()
	{
		return refl::descriptor::get_display_name_const(refl::type_descriptor<T>{}).str();
	}
}
