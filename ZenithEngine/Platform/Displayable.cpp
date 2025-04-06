#include "Displayable.h"

#include "Core/Assertion.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

namespace ZE::Platform
{
	static void GLFWErrorCallback(int error, const char* pDescription)
	{
		ZE_LOG_ERROR("GLFW error [{}]: {}", error, pDescription);
	}

	bool Displayble::Initialize()
	{
		ZE_CHECK(!m_IsInitialized);

		int result = glfwInit();
		m_IsInitialized = result == GLFW_TRUE;

		if (!glfwVulkanSupported())
		{
			m_IsInitialized = false;
			ZE_LOG_ERROR("Display device doesn't support vulkan!");
			glfwTerminate();
		}

		if (m_IsInitialized)
		{
			glfwSetErrorCallback(&GLFWErrorCallback);
		}
		
		return m_IsInitialized;
	}

	void Displayble::Shutdown()
	{
		if (m_IsInitialized)
		{
			glfwTerminate();
			m_IsInitialized = false;
		}
	}

	void Displayble::ProcessPlatformEvents()
	{
		glfwPollEvents();
	}
}