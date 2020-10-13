///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_LINUX_URING_QUEUE_HPP_INCLUDED
#define CPPCORO_DETAIL_LINUX_URING_QUEUE_HPP_INCLUDED

#include <cppcoro/config.hpp>

#include <cppcoro/detail/linux.hpp>

#include <chrono>
#include <mutex>

#include <liburing.h>

namespace cppcoro::detail::lnx
{
    template<typename _Rep, typename _Period>
    constexpr __kernel_timespec duration_to_eventspec(std::chrono::duration<_Rep, _Period> dur)
    {
        using namespace std::chrono;

        auto ns = duration_cast<nanoseconds>(dur);
        auto secs = duration_cast<seconds>(dur);
        ns -= secs;

        return {secs.count(), ns.count()};
    }

    template<typename _Clock, typename _Dur>
    constexpr __kernel_timespec time_point_to_eventspec(std::chrono::time_point<_Clock, _Dur> tp)
    {
        using namespace std::chrono;

        auto secs = time_point_cast<seconds>(tp);
        auto ns = time_point_cast<nanoseconds>(tp) -
                  time_point_cast<nanoseconds>(secs);

        return {secs.time_since_epoch().count(), ns.count()};
    }

	class uring_queue
	{
	public:
		explicit uring_queue(size_t queue_length = 32, uint32_t flags = 0);
		~uring_queue() noexcept;
		uring_queue(uring_queue&&) = delete;
		uring_queue& operator=(uring_queue&&) = delete;
		uring_queue(uring_queue const&) = delete;
		uring_queue& operator=(uring_queue const&) = delete;
		bool dequeue(io_message*& message, bool wait);
		int submit() noexcept;
		io_uring_sqe* get_sqe() noexcept;

	private:
		std::mutex m_inMux;
		std::mutex m_outMux;
		io_uring ring_{};
	};
	using io_queue = uring_queue;
}  // namespace cppcoro::detail::lnx

#endif // CPPCORO_DETAIL_LINUX_URING_QUEUE_HPP_INCLUDED
