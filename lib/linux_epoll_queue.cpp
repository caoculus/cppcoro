#include <cppcoro/detail/linux_epoll_queue.hpp>

#include <system_error>
#include <sys/timerfd.h>

namespace cppcoro::detail::lnx {
    io_transaction::io_transaction(epoll_queue &queue, io_message &message) noexcept
        : m_queue{queue}, m_message{message} {
        m_message.event.events = EPOLLIN;
    }

    bool io_transaction::commit() noexcept {
        if (m_error) {
            m_message.result = m_error;
            return false;
        }
        if (m_queue.submit(*m_message.fd, &m_message.event) < 0) {
            m_message.result = -errno;
            return false;
        }
        return true;
    }

    io_transaction &io_transaction::timeout(timespec *ts, bool absolute) noexcept {

        m_message.fd = safe_handle{ timerfd_create(CLOCK_MONOTONIC, 0) };
        if (!m_message.fd)
        {
            m_error = -errno;
            return *this;
        }
        itimerspec timerspec{{0, 0}, *ts};
        if (timerfd_settime(*m_message.fd, 0, &timerspec, nullptr) < 0) {
            m_error = -errno;
        }
        return *this;
    }
    io_transaction &io_transaction::nop() noexcept {
        m_message.fd = safe_handle{ eventfd(1, 0) };
        if (!m_message.fd)
        {
            m_error = -errno;
        }
        return *this;
    }

    io_transaction &io_transaction::cancel(int flags) noexcept {
        // dont use epoll_operation_base::submit here !
        if (m_queue.submit(*m_message.fd, &m_message.event, EPOLL_CTL_DEL)) {
            m_error = -errno;
            return *this;
        }
        m_message.fd = safe_handle{ eventfd(1, 0) };
        if (!m_message.fd)
        {
            m_error = -errno;
            return *this;
        }
        m_message.result = -ECANCELED;
        return *this;
    }

    epoll_queue::epoll_queue(size_t queue_length, uint32_t /*flags*/) {
        m_epollFd = safe_fd{epoll_create(queue_length)};
        if (!m_epollFd) {
            throw std::system_error{-errno, std::system_category(), "during epoll_create"};
        }
    }

    int epoll_queue::submit(int fd, epoll_event *event, int op) noexcept {
        // edge-triggered event
        // see https://man7.org/linux/man-pages/man7/epoll.7.html
        event->events |= EPOLLET;
        return epoll_ctl(*m_epollFd, op, fd, event);
    }

    bool epoll_queue::dequeue(detail::lnx::io_message *&msg, bool wait) {
        std::lock_guard guard(m_outMux);
        epoll_event event{0, {nullptr}};
        int ret = epoll_wait(*m_epollFd, &event, 1, wait ? -1 : 0);
        if (ret == -EAGAIN) {
            return false;
        } else if (ret < 0) {
            throw std::system_error{-ret,
                                    std::system_category(),
                                    std::string{"during epoll_wait"}};
        } else if (ret == 0) {
            return false; // no event
        } else {
            msg = reinterpret_cast<detail::lnx::io_message *>(event.data.ptr);
            if (msg != nullptr
                && msg->result == -1) // manually set result eg.: -ECANCEL
            {
                msg->result = ret;
            }
            return true;  // completed
        }
    }

    io_transaction epoll_queue::transaction(io_message &message) noexcept {
        return {*this, message};
    }

}  // namespace cppcoro::detail::lnx
