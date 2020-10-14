///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_LINUX_EPOLL_QUEUE_HPP_INCLUDED
#define CPPCORO_DETAIL_LINUX_EPOLL_QUEUE_HPP_INCLUDED

#include <cppcoro/config.hpp>

#include <cppcoro/detail/linux.hpp>

#include <chrono>
#include <mutex>
#include <sys/socket.h>

namespace cppcoro::detail::lnx
{
    class epoll_queue;

    /// RAII IO transaction
    class [[nodiscard]] io_transaction final {
    public:
        io_transaction(epoll_queue &queue, io_message& message) noexcept;
        bool commit() noexcept;

        [[nodiscard]] io_transaction &read(int fd, void *buffer, size_t size, size_t offset) noexcept;
        [[nodiscard]] io_transaction &write(int fd, const void * buffer, size_t size, size_t offset) noexcept;

        [[nodiscard]] io_transaction &readv(int fd, iovec* vec, size_t count, size_t offset) noexcept;
        [[nodiscard]] io_transaction &writev(int fd, iovec* vec, size_t count, size_t offset) noexcept;

        [[nodiscard]] io_transaction &recv(int fd, void * buffer, size_t size, int flags = 0) noexcept;
        [[nodiscard]] io_transaction &send(int fd, const void *buffer, size_t size, int flags = 0) noexcept;

        [[nodiscard]] io_transaction &recvmsg(int fd, msghdr *msg, int flags = 0) noexcept;
        [[nodiscard]] io_transaction &sendmsg(int fd, msghdr *msg, int flags = 0) noexcept;

        [[nodiscard]] io_transaction &connect(int fd, const void* to, size_t to_size) noexcept;
        [[nodiscard]] io_transaction &close(int fd) noexcept;

        [[nodiscard]] io_transaction &accept(int fd, const void* to, socklen_t* to_size, int flags = 0) noexcept;

        [[nodiscard]] io_transaction &timeout(timespec *ts, bool absolute = false) noexcept;
        [[nodiscard]] io_transaction &timeout_remove(int flags = 0) noexcept;

        [[nodiscard]] io_transaction &nop() noexcept;

        [[nodiscard]] io_transaction &cancel(int flags = 0) noexcept;

    private:
        epoll_queue &m_queue;
        io_message& m_message;
        int m_error = 0;
    };

    class epoll_queue
    {
    public:
        explicit epoll_queue(size_t queue_length = 32, uint32_t flags = 0);
        ~epoll_queue() noexcept = default;
        epoll_queue(epoll_queue&&) = delete;
        epoll_queue& operator=(epoll_queue&&) = delete;
        epoll_queue(epoll_queue const&) = delete;
        epoll_queue& operator=(epoll_queue const&) = delete;
        bool dequeue(io_message*& message, bool wait);
        io_transaction transaction(io_message& message) noexcept;

    private:
        friend class io_transaction;

        int submit(int fd, epoll_event* event, int op = EPOLL_CTL_ADD) noexcept;
        epoll_event* get_event() noexcept;

        std::mutex m_inMux;
        std::mutex m_outMux;
        safe_fd m_epollFd{};
    };
    using io_queue = epoll_queue;
}  // namespace cppcoro::detail::lnx

#endif // CPPCORO_DETAIL_LINUX_URING_QUEUE_HPP_INCLUDED
