#include "Input.h"

#include "Core/Assertion.h"
#include "InputCollector.h"

#include <GLFW/glfw3.h>

namespace ZE::Input
{
	void InputModule::CollectSystemInputs(ISystemInputCollector* pInputCollector)
	{
		ZE_ASSERT(m_IsInitialized);
	}
	
	bool InputModule::InitializeModule()
	{
		m_SystemInputCollector = new DefaultSystemInputCollector;

		if (!m_SystemInputCollector->Initialize())
		{
			delete m_SystemInputCollector;
			return false;
		}
		
		m_IsInitialized = true;
		return true;
	}
	
	void InputModule::ShutdownModule()
	{
		m_SystemInputCollector->Shutdown();
		delete m_SystemInputCollector;
	}
	
	void InputModule::AddWindow(Platform::Window* pWindow)
	{
		ZE_ASSERT(m_IsInitialized);

		m_SystemInputCollector->AddWindow(pWindow);
	}
	
	void InputModule::RemoveWindow(Platform::Window* pWindow)
	{
		ZE_ASSERT(m_IsInitialized); 

		m_SystemInputCollector->RemoveWindow(pWindow);
	}
}
