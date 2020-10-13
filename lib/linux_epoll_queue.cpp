#include <cppcoro/detail/linux_epoll_queue.hpp>

#include <system_error>

namespace cppcoro::detail::lnx
{
	epoll_queue::epoll_queue(size_t queue_length, uint32_t /*flags*/)
	{
		m_epollFd = safe_fd{ epoll_create(queue_length) };
		if (!m_epollFd)
		{
			throw std::system_error{ -errno, std::system_category(), "during epoll_create" };
		}
	}

	int epoll_queue::submit(int fd, epoll_event* event, int op) noexcept
	{
        // edge-triggered event
        // see https://man7.org/linux/man-pages/man7/epoll.7.html
	    event->events |= EPOLLET;
		return epoll_ctl(*m_epollFd, op, fd, event);
	}

	bool epoll_queue::dequeue(detail::lnx::io_message*& msg, bool wait)
	{
		std::lock_guard guard(m_outMux);
		epoll_event event{0, { nullptr}};
        int ret = epoll_wait(*m_epollFd, &event, 1, wait ? -1 : 0);
		if (ret == -EAGAIN)
		{
			return false;
		}
		else if (ret < 0)
		{
			throw std::system_error{ -ret,
									 std::system_category(),
									 std::string{ "during epoll_wait" } };
		}
		else if (ret == 0)
		{
			return false; // no event
		}
		else
		{
			msg = reinterpret_cast<detail::lnx::io_message*>(event.data.ptr);
			if (msg != nullptr
			    && msg->result == -1) // manually set result eg.: -ECANCEL
			{
				msg->result = ret;
			}
			return true;  // completed
		}
	}
}  // namespace cppcoro::detail::lnx
