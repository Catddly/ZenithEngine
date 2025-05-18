#pragma once

#include <unordered_map>

#include "Core/Event.h"

namespace ZE::Platform { class Window; }
namespace ZE::Input
{
	class ISystemInputCollector
	{
	public:

		virtual ~ISystemInputCollector() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void AddWindow(Platform::Window* pWindow) = 0;
		virtual void RemoveWindow(Platform::Window* pWindow) = 0;

	private:
	};

	class DefaultSystemInputCollector : public ISystemInputCollector
	{
	public:

		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual void AddWindow(Platform::Window* pWindow) override;
		virtual void RemoveWindow(Platform::Window* pWindow) override;
	
	private:

		// TODO: engine key and input format
		void OnReceiveInputEventFromWindow(Platform::Window* pWindow, int key, int action);
		
	private:

		std::unordered_map<Platform::Window*, Core::EventHandle>				m_ListeningWindowsMap;
	};
}
