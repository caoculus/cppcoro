///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_LINUX_HPP_INCLUDED
#define CPPCORO_DETAIL_LINUX_HPP_INCLUDED

#include <cppcoro/config.hpp>

#include <cstdint>
#include <cstdio>
#include <utility>

#include <fcntl.h>
#include <mqueue.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utility>

#include <liburing.h>

#include <string_view>
#include <mutex>
#include <cppcoro/coroutine.hpp>

#include <iostream> // TODO remove me !
#include <iomanip>

namespace cppcoro
{
	namespace detail
	{
		namespace lnx
		{
			using fd_t = int;

			class safe_fd
			{
			public:
				safe_fd()
					: m_fd(-1)
				{
				}

				explicit safe_fd(fd_t fd)
					: m_fd(fd)
				{
				}

				safe_fd(const safe_fd& other) = delete;

				safe_fd(safe_fd&& other) noexcept
					: m_fd(other.m_fd)
				{
					other.m_fd = -1;
				}

				~safe_fd() { close(); }

				safe_fd& operator=(safe_fd fd) noexcept
				{
					swap(fd);
					return *this;
				}

				constexpr fd_t fd() const { return m_fd; }
                constexpr fd_t handle() const { return m_fd; }

				/// Calls close() and sets the fd to -1.
				void close() noexcept;

				void swap(safe_fd& other) noexcept { std::swap(m_fd, other.m_fd); }

				bool operator==(const safe_fd& other) const { return m_fd == other.m_fd; }

				bool operator!=(const safe_fd& other) const { return m_fd != other.m_fd; }

				bool operator==(fd_t fd) const { return m_fd == fd; }

				bool operator!=(fd_t fd) const { return m_fd != fd; }

			private:
				fd_t m_fd;
			};

			struct io_message
			{
				void* awaitingCoroutine = nullptr;
				int   result = -1;
			};

			class uring_queue {
			public:
                explicit uring_queue(size_t queue_length = 32, uint32_t flags = 0);
                ~uring_queue() noexcept;
                uring_queue(uring_queue&&) = delete;
                uring_queue& operator=(uring_queue&&) = delete;
                uring_queue(uring_queue const&) = delete;
                uring_queue& operator=(uring_queue const&) = delete;
				bool dequeue(io_message*& message, bool wait);
                struct io_uring *handle() { return &ring_; }
                int submit() noexcept;
                io_uring_sqe *get_sqe() noexcept;

			private:
			    std::mutex m_inMux;
                std::mutex m_outMux;
                io_uring ring_{};
			};

			using io_queue = uring_queue;

            template<typename _Rep, typename _Period>
            constexpr __kernel_timespec duration_to_kernel_timespec(std::chrono::duration<_Rep, _Period> dur)
            {
                using namespace std::chrono;

                auto ns = duration_cast<nanoseconds>(dur);
                auto secs = duration_cast<seconds>(dur);
                ns -= secs;

                return {secs.count(), ns.count()};
            }

            template<typename _Clock, typename _Dur>
            constexpr __kernel_timespec time_point_to_kernel_timespec(std::chrono::time_point<_Clock, _Dur> tp)
            {
				using namespace std::chrono;

                auto secs = time_point_cast<seconds>(tp);
                auto ns = time_point_cast<nanoseconds>(tp) -
                          time_point_cast<nanoseconds>(secs);

                return {secs.time_since_epoch().count(), ns.count()};
            }

		}  // namespace linux

		using safe_handle = lnx::safe_fd;
        using dword_t = int;
		struct sock_buf {
            sock_buf(void *buf, size_t sz) : buffer(buf), size(sz) {}
			void * buffer;
			size_t size;
		};
		using handle_t = lnx::fd_t;
    }  // namespace detail
}  // namespace cppcoro

#endif // CPPCORO_DETAIL_LINUX_HPP_INCLUDED
