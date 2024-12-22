#pragma once

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
		*  
		*/
		void RequestClose();
		void Render();

		void* GetNativeHandle() const { return m_pWindow; }

	protected:

		Settings			m_Settings;

	private:

		GLFWwindow*			m_pWindow = nullptr;
	};
}
