#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

struct GLFWwindow;

namespace ZE::RenderBackend { class IRenderDevice; }

namespace ZE::Platform
{
	/* Window can only be created after IDisplayable is initialized. */
	class Window
	{
	public:

		struct Settings
		{
			uint32_t						m_Width = 1920u;
			uint32_t						m_Height = 1080u;
		};

		Window(const Settings& settings);
		virtual ~Window();

		/* Request this window to be close. Window will NOT be immediately destroyed.
		*/
		void RequestClose();
		bool IsRequestingClose() const;

		void* GetNativeHandle() const { return m_Window; }

		glm::uvec2 GetResolution() const { return {m_Settings.m_Width, m_Settings.m_Height}; }
		
		virtual void Resize(uint32_t width, uint32_t height);
	
	protected:

		Settings			m_Settings;

	private:

		GLFWwindow*			m_Window = nullptr;
	};
}
