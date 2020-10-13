///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_LINUX_URING_OPERATION_HPP_INCLUDED
#define CPPCORO_DETAIL_LINUX_URING_OPERATION_HPP_INCLUDED

#include <cppcoro/detail/linux_base_operation_cancellable.hpp>

#include <cppcoro/io_service.hpp>
#include <cppcoro/detail/linux.hpp>

#include <cppcoro/coroutine.hpp>
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <optional>
#include <system_error>

#include <cppcoro/detail/linux_uring_queue.hpp>

namespace cppcoro
{
	namespace detail
	{
	    static constexpr auto uring_no_sqe_available = -ENOSR;

		class uring_operation_base
		{
		public:
			uring_operation_base(lnx::io_queue& ioService, size_t offset = 0) noexcept
				: m_ioQueue(ioService)
				, m_offset(offset)
				, m_message{}
			{
			}

			bool try_start_read(int fd, void* buffer, size_t size) noexcept
			{
				m_vec.iov_base = buffer;
				m_vec.iov_len = size;
                return m_ioQueue.transaction(m_message)
                    .readv(fd, &m_vec, 1, m_offset)
                    .commit();
			}

			bool try_start_write(int fd, const void* buffer, size_t size) noexcept
			{
				m_vec.iov_base = const_cast<void*>(buffer);
				m_vec.iov_len = size;
                return m_ioQueue.transaction(m_message)
                    .writev(fd, &m_vec, 1, m_offset)
                    .commit();
			}

			bool try_start_send(int fd, const void* buffer, size_t size) noexcept
			{
                return m_ioQueue.transaction(m_message)
                    .send(fd, buffer, size)
                    .commit();
			}

			bool try_start_sendto(
				int fd, const void* to, size_t to_size, void* buffer, size_t size, int flags = 0) noexcept
			{
				m_vec.iov_base = buffer;
				m_vec.iov_len = size;
				std::memset(&m_msghdr, 0, sizeof(m_msghdr));
				m_msghdr.msg_name = const_cast<void*>(to);
				m_msghdr.msg_namelen = to_size;
				m_msghdr.msg_iov = &m_vec;
				m_msghdr.msg_iovlen = 1;
                return m_ioQueue.transaction(m_message)
                    .sendmsg(fd, &m_msghdr, flags)
                    .commit();
			}

			bool try_start_recv(int fd, void* buffer, size_t size, int flags) noexcept
			{
                return m_ioQueue.transaction(m_message)
                    .recv(fd, buffer, size, flags)
                    .commit();
			}

			bool try_start_recvfrom(
				int fd, void* from, size_t from_size, void* buffer, size_t size, int flags = 0) noexcept
			{
				m_vec.iov_base = buffer;
				m_vec.iov_len = size;
				std::memset(&m_msghdr, 0, sizeof(m_msghdr));
				m_msghdr.msg_name = from;
				m_msghdr.msg_namelen = from_size;
				m_msghdr.msg_iov = &m_vec;
				m_msghdr.msg_iovlen = 1;
                return m_ioQueue.transaction(m_message)
                    .sendmsg(fd, &m_msghdr, flags)
                    .commit();
			}

			bool try_start_connect(int fd, const void* to, size_t to_size) noexcept
			{
                return m_ioQueue.transaction(m_message)
                    .connect(fd, to, to_size)
                    .commit();
			}

			bool try_start_disconnect(int fd) noexcept
			{
                return m_ioQueue.transaction(m_message)
                    .close(fd)
                    .commit();
			}

			bool try_start_accept(int fd, const void* to, socklen_t* to_size, int flags = 0) noexcept
			{
                return m_ioQueue.transaction(m_message)
                    .accept(fd, to, to_size, flags)
                    .commit();
			}

			bool try_start_timeout(__kernel_timespec *ts, bool absolute = false) noexcept {
                return m_ioQueue.transaction(m_message)
                    .timeout(ts)
                    .commit();
			}

            bool timeout_remove(int flags = 0)
            {
                return m_ioQueue.transaction(m_message)
                    .timeout_remove(flags)
                    .commit();
            }

            bool try_start_nop() noexcept {
                return m_ioQueue.transaction(m_message)
                    .nop()
                    .commit();
            }

			bool cancel_io(int flags = 0)
			{
                return m_ioQueue.transaction(m_message)
                    .cancel(flags)
                    .commit();
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
			iovec m_vec;
			msghdr m_msghdr;
			detail::lnx::io_message m_message;
            detail::lnx::io_queue& m_ioQueue;
		};

		template<typename OPERATION>
		class uring_operation : protected uring_operation_base
		{
		protected:
			uring_operation(lnx::io_queue& ioService, size_t offset = 0) noexcept
				: uring_operation_base(ioService, offset)
			{
			}

		public:
			bool await_ready() const noexcept { return false; }

			CPPCORO_NOINLINE
			bool await_suspend(coroutine_handle<> awaitingCoroutine)
			{
				static_assert(std::is_base_of_v<uring_operation, OPERATION>);

				m_message = awaitingCoroutine;
				return static_cast<OPERATION*>(this)->try_start();
			}

			decltype(auto) await_resume() { return static_cast<OPERATION*>(this)->get_result(); }
		};

		using io_operation_base = uring_operation_base;

		template<typename OPERATION>
		using io_operation = uring_operation<OPERATION>;

		template<typename OPERATION>
		using io_operation_cancellable = base_operation_cancellable<OPERATION, uring_operation<OPERATION>>;
	}  // namespace detail
}  // namespace cppcoro

#endif  // CPPCORO_DETAIL_LINUX_URING_OPERATION_HPP_INCLUDED
