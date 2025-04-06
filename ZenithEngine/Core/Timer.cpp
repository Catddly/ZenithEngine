#include "Timer.h"

namespace ZE::Core
{
	void Timer::Start()
	{
		if (m_Paused)
		{
			m_StartTime = std::chrono::high_resolution_clock::now();
			m_Paused = false;
		}
	}
	
	void Timer::Pause()
	{
		if (!m_Paused)
		{
			m_TotalElapsedTime += m_LastUpdatedTime - m_StartTime;
			m_Paused = true;	
		}
	}

	void Timer::Reset()
	{
		m_TotalElapsedTime = {};
		m_Paused = true;
	}
	
	void Timer::Tick()
	{
		if (!m_Paused)
		{
			m_LastUpdatedTime = std::chrono::high_resolution_clock::now();
		}
	}
}
