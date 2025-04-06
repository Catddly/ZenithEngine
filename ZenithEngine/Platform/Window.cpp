#include "Window.h"

#include "Core/Assertion.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

namespace ZE::Platform
{
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		Window* pWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		pWindow->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}
	
	Window::Window(const Settings& settings)
		: m_Settings(settings)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Window = glfwCreateWindow(
			static_cast<int>(m_Settings.m_Width),
			static_cast<int>(m_Settings.m_Height),
			"Zenith Engine", nullptr, nullptr);

		if (!m_Window)
		{
			ZE_LOG_FATAL("Failed to create window!");
		}

		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, &FramebufferResizeCallback);
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_Window);
		m_Window = nullptr;
	}

	void Window::RequestClose()
	{
		if (m_Window)
		{
			glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
		}
	}
	
	bool Window::IsRequestingClose() const
	{
		return glfwWindowShouldClose(m_Window);
	}

	void Window::Resize(uint32_t width, uint32_t height)
	{
		if ((width != m_Settings.m_Width || height != m_Settings.m_Height) &&
			width != 0 && height != 0)
		{
			m_Settings.m_Width = width;
			m_Settings.m_Height = height;
		}
	}
}
