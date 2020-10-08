///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Sylvain Garcia
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_ASYNC_THREAD_HPP_INCLUDED
#define CPPCORO_ASYNC_THREAD_HPP_INCLUDED

#include <cppcoro/io_service.hpp>
#include <cppcoro/async_io_event.hpp>

#include <functional>
#include <thread>

namespace cppcoro
{
	class async_thread
	{
	public:
		/** @brief Awaitable thread
		 * @details
		 *
		 * Starts the given @a callable in a new thread.
		 * The thread must be joined with `co_await thread.join()`.
		 *
		 * @tparam CALLABLE A callable type
		 * @param io_service The executor
		 * @param callable The callable object
		 */
		template<typename CALLABLE>
		async_thread(io_service& io_service, CALLABLE&& callable) noexcept
			: m_event{ io_service }
			, m_thread{ [this, callable = std::forward<CALLABLE>(callable)]() mutable {
				std::invoke(std::forward<CALLABLE>(callable));
                m_event.set();
			} }
		{
		}

		template<typename CALLABLE>
		async_thread(io_service& io_service, CALLABLE&& callable, cancellation_token ct) noexcept
			: m_event{ io_service }
			, m_thread{
				[this, callable = std::forward<CALLABLE>(callable), ct = std::move(ct)]() mutable {
					std::invoke(std::forward<CALLABLE>(callable), std::move(ct));
                    m_event.set();
				}
			}
		{
		}

		~async_thread() noexcept
		{
			// should not be joinable anymore but...
			if (m_thread.joinable())
				m_thread.join();
		}

		decltype(auto) join()
		{
            return m_event.operator co_await();
		}

	private:
		async_io_event m_event;
		std::thread m_thread;
	};
}  // namespace cppcoro

#endif  // CPPCORO_ASYNC_THREAD_HPP_INCLUDED
