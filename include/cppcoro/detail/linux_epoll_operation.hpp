///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_LINUX_EPOLL_OPERATION_HPP_INCLUDED
#define CPPCORO_DETAIL_LINUX_EPOLL_OPERATION_HPP_INCLUDED

#include <cppcoro/cancellation_registration.hpp>
#include <cppcoro/cancellation_token.hpp>
#include <cppcoro/operation_cancelled.hpp>

#include <cppcoro/io_service.hpp>
#include <cppcoro/detail/linux.hpp>

#include <cppcoro/coroutine.hpp>
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <optional>
#include <system_error>

#include <cppcoro/detail/linux_epoll_queue.hpp>

#include <sys/timerfd.h>

namespace cppcoro::detail
{
	class epoll_operation_base
	{
		auto submitt()
		{
			m_message.awaitingCoroutine = m_awaitingCoroutine.address();
			return m_ioQueue.submit(*m_fd, &m_event);
		}

	public:
		epoll_operation_base(lnx::io_queue& ioService, size_t offset = 0) noexcept
			: m_ioQueue(ioService)
			, m_offset(offset)
			, m_message{}
		{
		}
		//
		//        bool try_start_read(int fd, void* buffer, size_t size) noexcept
		//        {
		//            m_vec.iov_base = buffer;
		//            m_vec.iov_len = size;
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_readv(sqe, fd, &m_vec, 1, m_offset);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_write(int fd, const void* buffer, size_t size) noexcept
		//        {
		//            m_vec.iov_base = const_cast<void*>(buffer);
		//            m_vec.iov_len = size;
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_writev(sqe, fd, &m_vec, 1, m_offset);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_send(int fd, const void* buffer, size_t size) noexcept
		//        {
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_send(sqe, fd, buffer, size, 0);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_sendto(
		//            int fd, const void* to, size_t to_size, void* buffer, size_t size) noexcept
		//        {
		//            m_vec.iov_base = buffer;
		//            m_vec.iov_len = size;
		//            std::memset(&m_msghdr, 0, sizeof(m_msghdr));
		//            m_msghdr.msg_name = const_cast<void*>(to);
		//            m_msghdr.msg_namelen = to_size;
		//            m_msghdr.msg_iov = &m_vec;
		//            m_msghdr.msg_iovlen = 1;
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_sendmsg(sqe, fd, &m_msghdr, 0);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_recv(int fd, void* buffer, size_t size, int flags) noexcept
		//        {
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_recv(sqe, fd, buffer, size, flags);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_recvfrom(
		//            int fd, void* from, size_t from_size, void* buffer, size_t size, int flags)
		//            noexcept
		//        {
		//            m_vec.iov_base = buffer;
		//            m_vec.iov_len = size;
		//            std::memset(&m_msghdr, 0, sizeof(m_msghdr));
		//            m_msghdr.msg_name = from;
		//            m_msghdr.msg_namelen = from_size;
		//            m_msghdr.msg_iov = &m_vec;
		//            m_msghdr.msg_iovlen = 1;
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_recvmsg(sqe, fd, &m_msghdr, flags);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_connect(int fd, const void* to, size_t to_size) noexcept
		//        {
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_connect(
		//                sqe, fd, reinterpret_cast<sockaddr*>(const_cast<void*>(to)), to_size);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_disconnect(int fd) noexcept
		//        {
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_close(sqe, fd);
		//            return submitt(sqe) == 1;
		//        }
		//
		//        bool try_start_accept(int fd, const void* to, socklen_t* to_size) noexcept
		//        {
		//            auto sqe = m_ioQueue.get_sqe();
		//            io_uring_prep_accept(
		//                sqe, fd, reinterpret_cast<sockaddr*>(const_cast<void*>(to)), to_size, 0);
		//            return submitt(sqe) == 1;
		//        }

		bool try_start_timeout(itimerspec* ts, bool absolute = false) // TODO noexcept
		{
			m_fd = safe_handle{ timerfd_create(CLOCK_MONOTONIC, 0) };
			if (!m_fd)
			{
				throw std::system_error { -errno, std::system_category(), "during timerfd_create" };
			}

            if (timerfd_settime(*m_fd, 0, ts, nullptr) < 0) {
                throw std::system_error { -errno, std::system_category(), "during timerfd_settime" };
            }
			return submitt() == 0;
		}

		bool try_start_nop()
		{
            m_fd = safe_handle{ eventfd(1, 0) };
            if (!m_fd)
            {
                throw std::system_error { -errno, std::system_category(), "during eventfd" };
            }
            return submitt() == 0;
		}

		bool cancel_io()
		{
		    bool success = m_ioQueue.submit(*m_fd, &m_event, EPOLL_CTL_DEL) == 0;  // dont use epoll_operation_base::submit here !
            m_fd = safe_handle{ eventfd(1, 0) };
            if (!m_fd)
            {
                throw std::system_error { -errno, std::system_category(), "during eventfd" };
            }
            m_message.result = -ECANCELED;
            return success | (m_ioQueue.submit(*m_fd, &m_event) == 0);
		}

		std::size_t get_result()
		{
			if (m_message.result < 0)
			{
				throw std::system_error{ -m_message.result, std::system_category() };
			}

			return m_message.result;
		}

		size_t m_offset;
		coroutine_handle<> m_awaitingCoroutine;
		safe_handle m_fd;
        detail::lnx::io_message m_message;
		epoll_event m_event {
			EPOLLIN,
            {&m_message}
		};

		iovec m_vec;
		msghdr m_msghdr;
		detail::lnx::io_queue& m_ioQueue;
	};

	template<typename OPERATION>
	class epoll_operation : protected epoll_operation_base
	{
	protected:
		epoll_operation(lnx::io_queue& ioService, size_t offset = 0) noexcept
			: epoll_operation_base(ioService, offset)
		{
		}

	public:
		bool await_ready() const noexcept { return false; }

		CPPCORO_NOINLINE
		bool await_suspend(coroutine_handle<> awaitingCoroutine)
		{
			static_assert(std::is_base_of_v<epoll_operation, OPERATION>);

			m_awaitingCoroutine = awaitingCoroutine;
			return static_cast<OPERATION*>(this)->try_start();
		}

		decltype(auto) await_resume() { return static_cast<OPERATION*>(this)->get_result(); }
	};

	template<typename OPERATION, typename BASE_OPERATION>
	class epoll_operation_cancellable : protected BASE_OPERATION
	{
		// ERROR_OPERATION_ABORTED value from <errno.h>
		static constexpr int error_operation_aborted = -ECANCELED;

	protected:
		epoll_operation_cancellable(lnx::io_queue& ioQueue, cancellation_token&& ct) noexcept
			: BASE_OPERATION(ioQueue, 0)
			, m_state(ct.is_cancellation_requested() ? state::completed : state::not_started)
			, m_cancellationToken(std::move(ct))
		{
		}

		epoll_operation_cancellable(
			lnx::io_queue& ioQueue, size_t offset, cancellation_token&& ct) noexcept
			: BASE_OPERATION(ioQueue, offset)
			, m_state(ct.is_cancellation_requested() ? state::completed : state::not_started)
			, m_cancellationToken(std::move(ct))
		{
		}

	public:
		bool await_ready() const noexcept
		{
			return m_state.load(std::memory_order_relaxed) == state::completed;
		}

		CPPCORO_NOINLINE
		bool await_suspend(coroutine_handle<> awaitingCoroutine)
		{
			static_assert(std::is_base_of_v<epoll_operation_cancellable, OPERATION>);

            this->m_awaitingCoroutine = awaitingCoroutine;

			// TRICKY: Register cancellation callback before starting the operation
			// in case the callback registration throws due to insufficient
			// memory. We need to make sure that the logic that occurs after
			// starting the operation is noexcept, otherwise we run into the
			// problem of not being able to cancel the started operation and
			// the dilemma of what to do with the exception.
			//
			// However, doing this means that the cancellation callback may run
			// prior to returning below so in the case that cancellation may
			// occur we defer setting the state to 'started' until after
			// the operation has finished starting. The cancellation callback
			// will only attempt to request cancellation of the operation with
			// CancelIoEx() once the state has been set to 'started'.
			if (m_cancellationToken.is_cancellation_requested())
			{
                this->m_message.result = error_operation_aborted;
				return false;
			}

			const bool canBeCancelled = m_cancellationToken.can_be_cancelled();
			m_state.store(state::started, std::memory_order_relaxed);

			// Now start the operation.
			const bool willCompleteAsynchronously = static_cast<OPERATION*>(this)->try_start();
			if (!willCompleteAsynchronously)
			{
				// Operation completed synchronously, resume awaiting coroutine immediately.
                this->m_message.result = error_operation_aborted;
				return false;
			}

			if (canBeCancelled)
			{
				// Need to flag that the operation has finished starting now.

				// However, the operation may have completed concurrently on
				// another thread, transitioning directly from not_started -> complete.
				// Or it may have had the cancellation callback execute and transition
				// from not_started -> cancellation_requested. We use a compare-exchange
				// to determine a winner between these potential racing cases.
				state oldState = state::not_started;
				if (!m_state.compare_exchange_strong(
						oldState,
						state::started,
						std::memory_order_release,
						std::memory_order_acquire))
				{
					if (oldState == state::cancellation_requested)
					{
						// Request the operation be cancelled.
						// Note that it may have already completed on a background
						// thread by now so this request for cancellation may end up
						// being ignored.
						static_cast<OPERATION*>(this)->cancel();

						if (!m_state.compare_exchange_strong(
								oldState,
								state::started,
								std::memory_order_release,
								std::memory_order_acquire))
						{
							assert(oldState == state::completed);
                            this->m_message.result = error_operation_aborted;
							return false;
						}
					}
					else
					{
						m_cancellationRegistration.emplace(std::move(m_cancellationToken), [this] {
							m_state.store(state::cancellation_requested, std::memory_order_seq_cst);
							static_cast<OPERATION*>(this)->cancel();
						});
						assert(oldState == state::started);
						return true;
					}
				}
			}

			return true;
		}

		decltype(auto) await_resume()
		{
			if (this->m_message.result == error_operation_aborted)
			{
				throw operation_cancelled{};
			}
			else if (this->m_message.result < 0)
			{
				if (this->m_message.result == -EINTR &&
					m_state.load(std::memory_order_acquire) == state::cancellation_requested)
				{
					throw operation_cancelled{};
				}
				throw std::system_error{ -this->m_message.result, std::system_category() };
			}

			return static_cast<OPERATION*>(this)->get_result();
		}

	private:
		enum class state
		{
			not_started,
			started,
			cancellation_requested,
			completed
		};

		void on_cancellation_requested() noexcept
		{
			auto oldState = m_state.load(std::memory_order_acquire);
			if (oldState == state::not_started)
			{
				// This callback is running concurrently with await_suspend().
				// The call to start the operation may not have returned yet so
				// we can't safely request cancellation of it. Instead we try to
				// notify the await_suspend() thread by transitioning the state
				// to state::cancellation_requested so that the await_suspend()
				// thread can request cancellation after it has finished starting
				// the operation.
				const bool transferredCancelResponsibility = m_state.compare_exchange_strong(
					oldState,
					state::cancellation_requested,
					std::memory_order_release,
					std::memory_order_acquire);
				if (transferredCancelResponsibility)
				{
					return;
				}
			}

			// No point requesting cancellation if the operation has already completed.
			if (oldState != state::completed)
			{
				static_cast<OPERATION*>(this)->cancel();
			}
		}

		std::atomic<state> m_state;
		cppcoro::cancellation_token m_cancellationToken;
		std::optional<cppcoro::cancellation_registration> m_cancellationRegistration;
	};

	using io_operation_base = epoll_operation_base;

	template<typename OPERATION>
	using io_operation = epoll_operation<OPERATION>;

	template<typename OPERATION>
	using io_operation_cancellable =
		epoll_operation_cancellable<OPERATION, epoll_operation<OPERATION>>;
}  // namespace cppcoro::detail

#endif  // CPPCORO_DETAIL_LINUX_EPOLL_OPERATION_HPP_INCLUDED
