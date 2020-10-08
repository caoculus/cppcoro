///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Sylvain Garcia
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_IO_EVENT_HPP_INCLUDED
#define CPPCORO_IO_EVENT_HPP_INCLUDED

#include <cppcoro/cancellation_token.hpp>
#include <cppcoro/io_service.hpp>
#include <cppcoro/detail/linux_uring_operation.hpp>

#include <functional>
#include <thread>

namespace cppcoro
{
	class async_io_event_wait_operation
		: public cppcoro::detail::io_operation<async_io_event_wait_operation>
	{
	public:
		async_io_event_wait_operation(
			io_service& io_service, int event, std::mutex& mux, bool& is_set) noexcept
			: cppcoro::detail::io_operation<async_io_event_wait_operation>{ io_service }
			, m_event{ event }
			, m_isSet{ is_set }
			, m_mux{ mux }
		{
		}

		bool is_ready() const noexcept
		{
			m_mux.lock();
			if (m_isSet)
			{
				//
				// event is set:
				//   - unlock and get result immediately (aka.: get_result())
				//
				m_mux.unlock();
				return true;
			}
			else
			{
				//
				// event is not set:
				//   - keep the lock until event read operation have been
				//     push onto aio queue (aka.: try_start())
				//
				return false;
			}
		}

		bool try_start() noexcept
		{
			bool result = try_start_event_read(m_event, &m_data);
			m_mux.unlock();
			return result;
		}

		void get_result() {}

	private:
		friend cppcoro::detail::io_operation<async_io_event_wait_operation>;

		int m_event;
		uint64_t m_data;
		bool& m_isSet;
		std::mutex& m_mux;
	};

	class async_io_event
	{
	public:
		/** @brief Awaitable io event
		 * @details
		 *
		 * In opposition to async_auto_reset_event and async_manual_reset_event,
		 * async_io_event is a THREAD synchronisation abstraction
		 * that allows one coroutine to wait until some thread calls
		 * set() on the event.
		 *
		 * The coroutine waiting on join() is resumed by the @a io_service.
		 *
		 * @param io_service The executor
		 */
		async_io_event(io_service& io_service) noexcept
			: m_event{ eventfd(0, EFD_CLOEXEC) }
			, m_ioService{ io_service }
		{
		}

		void set() noexcept
		{
			std::scoped_lock scoped_lock{ m_mux };
			m_isSet = true;
			uint64_t dumb = 0;
			write(m_event.handle(), &dumb, sizeof(dumb));
		}

		decltype(auto) operator co_await()
		{
			return async_io_event_wait_operation{ m_ioService, m_event.handle(), m_mux, m_isSet };
		}

	private:
		bool m_isSet{ false };
		std::mutex m_mux{};
		io_service& m_ioService;
		detail::safe_handle m_event;
		std::thread m_thread;
	};
}  // namespace cppcoro

#endif  // CPPCORO_IO_EVENT_HPP_INCLUDED
