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

namespace cppcoro::detail::lnx
{
    template<typename _Rep, typename _Period>
    constexpr itimerspec duration_to_eventspec(std::chrono::duration<_Rep, _Period> dur)
    {
        using namespace std::chrono;

        auto ns = duration_cast<nanoseconds>(dur);
        auto secs = duration_cast<seconds>(dur);
        ns -= secs;

        return {{0, 0}, { secs.count(), ns.count() } };
    }

    template<typename _Clock, typename _Dur>
    constexpr itimerspec time_point_to_eventspec(std::chrono::time_point<_Clock, _Dur> tp)
    {
        using namespace std::chrono;

        auto secs = time_point_cast<seconds>(tp);
        auto ns = time_point_cast<nanoseconds>(tp) -
                  time_point_cast<nanoseconds>(secs);

        return {{0, 0}, { secs.time_since_epoch().count(), ns.count() } };
    }

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
        int submit(int fd, epoll_event* event, int op = EPOLL_CTL_ADD) noexcept;
        epoll_event* get_event() noexcept;

    private:
        std::mutex m_inMux;
        std::mutex m_outMux;
        safe_fd m_epollFd{};
    };
    using io_queue = epoll_queue;
}  // namespace cppcoro::detail::lnx

#endif // CPPCORO_DETAIL_LINUX_URING_QUEUE_HPP_INCLUDED
