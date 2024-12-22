#pragma once

#include "Window.h"

#include <memory>

namespace ZE::Platform
{
	class IDisplayable
	{
	public:

		virtual ~IDisplayable() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void ProcessPlatformEvents() = 0;

	private:


	};
}