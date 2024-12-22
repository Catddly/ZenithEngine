#include "Window.h"

#include "Core/Assertion.h"
#include "Log/Log.h"

#include "GLFW/glfw3.h"

namespace ZE::Platform
{
	Window::Window(const Settings& settings)
		: m_Settings(settings)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_pWindow = glfwCreateWindow(m_Settings.m_Width, m_Settings.m_Height, "Zenith Engine", nullptr, nullptr);

		if (!m_pWindow)
		{
			ZE_LOG_FATAL("Failed to create window!");
		}
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	void Window::RequestClose()
	{
		if (m_pWindow)
		{
			glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
		}
	}

	void Window::Render()
	{
		if (!glfwWindowShouldClose(m_pWindow))
		{
		}
	}
}