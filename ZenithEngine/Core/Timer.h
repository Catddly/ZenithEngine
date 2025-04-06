#pragma once

#include <chrono>

namespace ZE::Core
{
	enum class ETimeUnit : uint8_t
	{
		Second,
		MilliSecond,
		MicroSecond,
		NanoSecond
	};
	
	class Timer
	{
	public:
		
		void Start();
		void Pause();
		void Reset();

		void Tick();

		template <ETimeUnit Unit>
		double GetTotalElapsedTime() const;

		template <ETimeUnit Unit>
		double GetLastElapsedTime() const;
		
	private:

		std::chrono::high_resolution_clock::time_point			m_StartTime;
		std::chrono::high_resolution_clock::time_point			m_LastUpdatedTime;

		std::chrono::duration<double>							m_TotalElapsedTime = {};

		bool													m_Paused = true;
	};
	
	template <ETimeUnit Unit>
	double Timer::GetTotalElapsedTime() const
	{
		using namespace std::chrono;

		const duration<double> lastElapsedTime = m_LastUpdatedTime - m_StartTime;
		
		if constexpr (Unit == ETimeUnit::Second)
		{
			return (m_TotalElapsedTime + lastElapsedTime).count();
		}
		else if constexpr (Unit == ETimeUnit::MilliSecond)
		{
			return duration<double, std::milli>(m_TotalElapsedTime + lastElapsedTime).count();
		}
		else if constexpr (Unit == ETimeUnit::MicroSecond)
		{
			return duration<double, std::micro>(m_TotalElapsedTime + lastElapsedTime).count();
		}
		else if constexpr (Unit == ETimeUnit::NanoSecond)
		{
			return duration<double, std::nano>(m_TotalElapsedTime + lastElapsedTime).count();
		}
		return 0.0;
	}
	
	template <ETimeUnit Unit>
	double Timer::GetLastElapsedTime() const
	{
		using namespace std::chrono;
		
		const duration<double> lastElapsedTime = m_LastUpdatedTime - m_StartTime;

		if constexpr (Unit == ETimeUnit::Second)
		{
			return lastElapsedTime.count();
		}
		else if constexpr (Unit == ETimeUnit::MilliSecond)
		{
			return duration<double, std::milli>(lastElapsedTime).count();
		}
		else if constexpr (Unit == ETimeUnit::MicroSecond)
		{
			return duration<double, std::micro>(lastElapsedTime).count();
		}
		else if constexpr (Unit == ETimeUnit::NanoSecond)
		{
			return duration<double, std::nano>(lastElapsedTime).count();
		}
		return 0.0;
	}

	template <ETimeUnit Unit>
	class ScopedTimer : public Timer
	{
	public:

		explicit ScopedTimer(double& totalTimeElapsed);
		~ScopedTimer();

		ScopedTimer(const ScopedTimer&) = delete;
		ScopedTimer& operator=(const ScopedTimer&) = delete;
		ScopedTimer(ScopedTimer&&) = delete;
		ScopedTimer& operator=(ScopedTimer&&) = delete;
		
	private:

		double&					m_OutTotalTimeElapsed;
	};
	
	template <ETimeUnit Unit>
	ScopedTimer<Unit>::ScopedTimer(double& totalTimeElapsed)
		: m_OutTotalTimeElapsed(totalTimeElapsed)
	{
		Start();
	}

	template <ETimeUnit Unit>
	ScopedTimer<Unit>::~ScopedTimer()
	{
		Tick();
		m_OutTotalTimeElapsed = GetLastElapsedTime<Unit>();
	}
}
