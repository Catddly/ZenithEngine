#include "Displayable.h"

#include "Core/Assertion.h"
#include "Log/Log.h"
#include "Window.h"

#include "GLFW/glfw3.h"

namespace ZE::Platform
{
	static void GLFWErrorCallback(int error, const char* pDescription)
	{
		ZE_LOG_ERROR("GLFW error [{}]: {}", error, pDescription);
	}

	bool Displayble::Initialize()
	{
		ZE_CHECK(!m_bIsInitialized);

		int result = glfwInit();

		m_bIsInitialized = result == GLFW_TRUE;

		if (!glfwVulkanSupported())
		{
			m_bIsInitialized = false;
			ZE_LOG_ERROR("Display device doesn't support vulkan!");
			glfwTerminate();
		}

		if (m_bIsInitialized)
		{
			glfwSetErrorCallback(&GLFWErrorCallback);
		}

		return m_bIsInitialized;
	}

	void Displayble::Shutdown()
	{
		if (m_bIsInitialized)
		{
			glfwTerminate();
			m_bIsInitialized = false;
		}
	}

	void Displayble::ProcessPlatformEvents()
	{
		glfwPollEvents();
	}
}