#include "InputCollector.h"

#include "Platform/Window.h"

#include <GLFW/glfw3.h>

namespace ZE::Input
{
	bool DefaultSystemInputCollector::Initialize()
	{
		return true;
	}
	
	void DefaultSystemInputCollector::Shutdown()
	{
		
	}
	
	void DefaultSystemInputCollector::AddWindow(Platform::Window* pWindow)
	{
		if (!pWindow)
		{
			return;
		}
		
		if (!m_ListeningWindowsMap.contains(pWindow))
		{
			const auto handle = pWindow->OnReceiveInputEvent().Bind([this](Platform::Window* pWindow, int key, int action)
			{
				OnReceiveInputEventFromWindow(pWindow, key, action);
			});
			m_ListeningWindowsMap.emplace(pWindow, handle);
		}
	}
	
	void DefaultSystemInputCollector::RemoveWindow(Platform::Window* pWindow)
	{
		if (!pWindow)
		{
			return;
		}
		
		if (auto iter = m_ListeningWindowsMap.find(pWindow); iter != m_ListeningWindowsMap.end())
		{
			pWindow->OnReceiveInputEvent().Unbind(iter->second);
			m_ListeningWindowsMap.erase(iter);
		}
	}

	void DefaultSystemInputCollector::OnReceiveInputEventFromWindow(Platform::Window* pWindow, int key, int action)
	{
		if (action == GLFW_PRESS)
		{
			ZE_LOG_INFO("On Key Pressed: {}, {}", reinterpret_cast<void*>(pWindow), key);
		}
		else if (action == GLFW_RELEASE)
		{
			ZE_LOG_INFO("On Key Released: {}, {}", reinterpret_cast<void*>(pWindow), key);
		}
		else if (action == GLFW_REPEAT)
		{
			ZE_LOG_INFO("On Key Repeated: {}, {}", reinterpret_cast<void*>(pWindow), key);
		}
	}
}
