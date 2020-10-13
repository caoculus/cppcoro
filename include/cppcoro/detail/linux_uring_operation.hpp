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
		class uring_operation_base
		{
			auto submitt(io_uring_sqe* sqe)
			{
				m_message.awaitingCoroutine = m_awaitingCoroutine.address();
				io_uring_sqe_set_data(sqe, &m_message);
				return m_ioQueue.submit();
			}

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
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_readv(sqe, fd, &m_vec, 1, m_offset);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_write(int fd, const void* buffer, size_t size) noexcept
			{
				m_vec.iov_base = const_cast<void*>(buffer);
				m_vec.iov_len = size;
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_writev(sqe, fd, &m_vec, 1, m_offset);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_send(int fd, const void* buffer, size_t size) noexcept
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_send(sqe, fd, buffer, size, 0);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_sendto(
				int fd, const void* to, size_t to_size, void* buffer, size_t size) noexcept
			{
				m_vec.iov_base = buffer;
				m_vec.iov_len = size;
				std::memset(&m_msghdr, 0, sizeof(m_msghdr));
				m_msghdr.msg_name = const_cast<void*>(to);
				m_msghdr.msg_namelen = to_size;
				m_msghdr.msg_iov = &m_vec;
				m_msghdr.msg_iovlen = 1;
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_sendmsg(sqe, fd, &m_msghdr, 0);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_recv(int fd, void* buffer, size_t size, int flags) noexcept
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_recv(sqe, fd, buffer, size, flags);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_recvfrom(
				int fd, void* from, size_t from_size, void* buffer, size_t size, int flags) noexcept
			{
				m_vec.iov_base = buffer;
				m_vec.iov_len = size;
				std::memset(&m_msghdr, 0, sizeof(m_msghdr));
				m_msghdr.msg_name = from;
				m_msghdr.msg_namelen = from_size;
				m_msghdr.msg_iov = &m_vec;
				m_msghdr.msg_iovlen = 1;
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_recvmsg(sqe, fd, &m_msghdr, flags);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_connect(int fd, const void* to, size_t to_size) noexcept
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_connect(
					sqe, fd, reinterpret_cast<sockaddr*>(const_cast<void*>(to)), to_size);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_disconnect(int fd) noexcept
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_close(sqe, fd);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_accept(int fd, const void* to, socklen_t* to_size) noexcept
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_accept(
					sqe, fd, reinterpret_cast<sockaddr*>(const_cast<void*>(to)), to_size, 0);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
			}

			bool try_start_timeout(__kernel_timespec *ts, bool absolute = false) noexcept {
                auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_timeout(sqe, ts, 0, absolute ? IORING_TIMEOUT_ABS : 0);
				if (auto res = submitt(sqe); res < 0) {
				    m_message.result = res;
				    return false;
				}
				return true;
			}

            bool try_start_nop() noexcept {
                auto sqe = m_ioQueue.get_sqe();
                io_uring_prep_nop(sqe);
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
            }

			bool cancel_io()
			{
				auto sqe = m_ioQueue.get_sqe();
				io_uring_prep_cancel(sqe, &m_message, 0);
				io_uring_sqe_set_data(sqe, &m_message);
                m_message.result = -ECANCELED;
                if (auto res = submitt(sqe); res < 0) {
                    m_message.result = res;
                    return false;
                }
                return true;
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

				m_awaitingCoroutine = awaitingCoroutine;
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
