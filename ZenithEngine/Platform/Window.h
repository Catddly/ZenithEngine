#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

#include "Core/Event.h"

struct GLFWwindow;

namespace ZE::RenderBackend { class IRenderDevice; }

namespace ZE::Platform
{
	namespace _Impl
	{
		static void KeyCallbackFunc(GLFWwindow* window, int key, int scancode, int action, int mods);
	}
	
	/* Window can only be created after IDisplayable is initialized. */
	class Window
	{
	public:

		struct Settings
		{
			std::string						m_Name;
			
			uint32_t						m_Width = 1920u;
			uint32_t						m_Height = 1080u;
		};

		Window(const Settings& settings);
		virtual ~Window();

		Core::EventBinder<Window*, int, int> OnReceiveInputEvent() { return m_ReceiveInputEventEvent.ToBinder(); }
		
		/* Request this window to be close. Window will NOT be immediately destroyed.
		*/
		void RequestClose();
		bool IsCloseRequested() const;

		void* GetNativeHandle() const { return m_Window; }

		glm::uvec2 GetResolution() const { return {m_Settings.m_Width, m_Settings.m_Height}; }

		// Return true if a window actually needs to resize.
		// Return false if window resizing is failed or a window doesn't need to resize.
		virtual bool Resize(uint32_t width, uint32_t height);
	
	protected:

		Settings								m_Settings;

	private:
		
		friend static void _Impl::KeyCallbackFunc(GLFWwindow* window, int key, int scancode, int action, int mods);
		void AddKeyInputEvent(int key, int action);
	
	private:

		GLFWwindow*									m_Window = nullptr;
		Core::Event<Window*, int, int>				m_ReceiveInputEventEvent;
	};
}
