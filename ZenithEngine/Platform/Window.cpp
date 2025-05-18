#include "Window.h"

#include "Core/Assertion.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

namespace ZE::Platform
{
	namespace _Impl
	{
		static void KeyCallbackFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto* pWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
			pWindow->AddKeyInputEvent(key, action);
		}
	}
	
	namespace
	{
		void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
		{
			auto* pWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
			pWindow->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		}
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
		glfwSetKeyCallback(m_Window, &_Impl::KeyCallbackFunc);
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
	
	bool Window::IsCloseRequested() const
	{
		return glfwWindowShouldClose(m_Window);
	}

	bool Window::Resize(uint32_t width, uint32_t height)
	{
		if ((width != m_Settings.m_Width || height != m_Settings.m_Height) &&
			width != 0 && height != 0)
		{
			m_Settings.m_Width = width;
			m_Settings.m_Height = height;

			return true;
		}
		
		return false;
	}
	
	void Window::AddKeyInputEvent(int key, int action)
	{
		m_ReceiveInputEventEvent.Broadcast(this, key, action);
	}
}
