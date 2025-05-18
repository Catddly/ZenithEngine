#pragma once

#include "Core/ClassProperty.h"
#include "Hash.h"
#include "Log/Log.h"

#include <functional>
#include <vector>
#include <limits>

#include "Assertion.h"

namespace ZE::Core
{
	using EventTypeId = uint64_t;
	static constexpr EventTypeId gkInvalidEventTypeId = std::numeric_limits<uint64_t>::max();

	// Listener to a specific event. 
	class EventHandle
	{
		template <typename... Args>
		friend class Event;
		
	public:
		
		EventHandle() = default;

	private:

		EventHandle(EventTypeId typeId, int index)
			: m_TypeId{typeId}, m_Index{index}
		{}
		
	private:

		EventTypeId								m_TypeId = gkInvalidEventTypeId;
		int										m_Index = -1;
	};

	namespace _Impl
	{
		static EventTypeId AllocateEventTypeId()
		{
			static EventTypeId sId = 0;
			return sId++;
		}
	}

	// Class to only expose bind and unbind interface to user. Forbid user to call other functions.
	// User should be aware of the lifetime of this object. It should be destroyed before its owner (Event).
	template <typename... Args>
	class EventBinder
	{
		friend class Event<Args...>;
		
	public:

		EventHandle Bind(std::function<void(Args...)>&& func)
		{
			if (m_Event)
			{
				return m_Event->RegisterListener(std::move(func));	
			}

			ZE_UNREACHABLE();
			return {};
		}
		
		void Unbind(EventHandle& handle)
		{
			if (m_Event)
			{
				m_Event->UnregisterListener(handle);
				return;
			}
			
			ZE_UNREACHABLE();
		}

	private:

		EventBinder(Event<Args...>* pEvent)
			: m_Event{pEvent}
		{}

	private:

		Event<Args...>*				m_Event = nullptr;
	};
	
	// Event to broadcast information through function parameters.
	// This object is not a thread-safe object.
	template <typename... Args>
	class Event
	{
	public:
		
		Event()
			: m_TypeId(_Impl::AllocateEventTypeId())
		{}

		~Event()
		{
			if (!m_FuncObjects.empty())
			{
				ZE_ASSERT_LOG(false, "All listeners should be removed before this event is destroyed! Event: {}", m_TypeId);
			}
		}

		ZE_NON_COPYABLE_CLASS(Event);

		[[nodiscard]] inline EventBinder<Args...> ToBinder()
		{
			return EventBinder{this};
		}
		
		// Register a listen to this event.
		[[nodiscard]] EventHandle RegisterListener(std::function<void(Args...)>&& func)
		{
			auto handle = m_FuncObjects.size();
			m_FuncObjects.emplace_back(std::move(func));
			return EventHandle{m_TypeId, static_cast<int>(handle)};
		}
		
		// Forget a listener.
		void UnregisterListener(EventHandle& handle)
		{
			if (handle.m_TypeId != m_TypeId)
			{
				ZE_LOG_WARNING("Mismatch event type id between event handle and event. Expected: {}, Got: {}", m_TypeId, handle.m_TypeId);
				return;
			}
			
			if (handle.m_Index >= 0 && handle.m_TypeId != gkInvalidEventTypeId)
			{
				std::swap(m_FuncObjects.back(), m_FuncObjects[handle.m_Index]);
				m_FuncObjects.pop_back();

				handle.m_Index = -1;
			}

			// If an event is statically created, memory must be freed because the allocator is freed before static class instance
			if (m_FuncObjects.empty())
			{
				m_FuncObjects.shrink_to_fit();
			}
		}

		// Broadcast arguments to notify all listeners.
		// Calling this is NOT thread-safe, since event can call from any thread when other threads can enter the same function or R/W its states.
		// You should make sure functions are thread-safe if you want to.
		void Broadcast(Args... args) const
		{
			for (auto& func : m_FuncObjects)
			{
				if (func)
				{
					func(std::forward<Args>(args)...);
				}
			}
		}

		bool HasListeners() const { return !m_FuncObjects.empty(); }
		
	private:

		EventTypeId										m_TypeId = gkInvalidEventTypeId;
		std::vector<std::function<void(Args...)>>		m_FuncObjects;
	};
}
