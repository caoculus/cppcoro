#include <cppcoro/detail/linux_uring_queue.hpp>

namespace cppcoro::detail::lnx
{
	uring_queue::uring_queue(size_t queue_length, uint32_t flags)
	{
		auto err = io_uring_queue_init(queue_length, &ring_, flags);
		if (err < 0)
		{
			throw std::system_error{ static_cast<int>(-err),
									 std::system_category(),
									 "Error initializing uring" };
		}
	}

	uring_queue::~uring_queue() noexcept { io_uring_queue_exit(&ring_); }

	io_uring_sqe* uring_queue::get_sqe() noexcept
	{
		m_inMux.lock();
		return io_uring_get_sqe(&ring_);
	}

	int uring_queue::submit() noexcept
	{
		int res = io_uring_submit(&ring_);
		m_inMux.unlock();
		return res;
	}

	bool uring_queue::dequeue(detail::lnx::io_message*& msg, bool wait)
	{
		std::lock_guard guard(m_outMux);
		io_uring_cqe* cqe;
		int ret;
		if (wait)
			ret = io_uring_wait_cqe(&ring_, &cqe);
		else
			ret = io_uring_peek_cqe(&ring_, &cqe);
		if (ret == -EAGAIN)
		{
			return false;
		}
		else if (ret < 0)
		{
			throw std::system_error{ -ret,
									 std::system_category(),
									 std::string{ "io_uring_peek_cqe failed" } };
		}
		else
		{
			msg = reinterpret_cast<detail::lnx::io_message*>(io_uring_cqe_get_data(cqe));
			if (msg != nullptr)
			{
				msg->result = cqe->res;
			}
			io_uring_cqe_seen(&ring_, cqe);
			return true;  // completed
		}
	}
}  // namespace cppcoro::detail::lnx
