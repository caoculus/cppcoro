///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_FILE_READ_OPERATION_HPP_INCLUDED
#define CPPCORO_FILE_READ_OPERATION_HPP_INCLUDED

#include <cppcoro/config.hpp>
#include <cppcoro/cancellation_registration.hpp>
#include <cppcoro/cancellation_token.hpp>

#include <cppcoro/detail/stdcoro.hpp>
#include <atomic>
#include <optional>

#if CPPCORO_OS_WINNT
#include <cppcoro/detail/win32.hpp>
#include <cppcoro/detail/win32_overlapped_operation.hpp>
#elif CPPCORO_OS_LINUX

#include <cppcoro/detail/linux_uring_operation.hpp>

#endif

namespace cppcoro
{
	class file_read_operation_impl
	{
	public:
		file_read_operation_impl(
			detail::handle_t fileHandle, void* buffer, std::size_t byteCount) noexcept
			: m_fileHandle(fileHandle)
			, m_buffer(buffer)
			, m_byteCount(byteCount)
		{}

		bool try_start(cppcoro::detail::io_operation_base& operation) noexcept;
		void cancel(cppcoro::detail::io_operation_base& operation) noexcept;

	private:
		detail::handle_t m_fileHandle;
		void* m_buffer;
		std::size_t m_byteCount;
	};

	class file_read_operation : public cppcoro::detail::io_operation<file_read_operation>
	{
	public:
		file_read_operation(
#if CPPCORO_OS_LINUX
			io_service& ioService,
#endif
			detail::handle_t fileHandle,
			std::uint64_t fileOffset,
			void* buffer,
			std::size_t byteCount) noexcept
			: cppcoro::detail::io_operation<file_read_operation>(
#if CPPCORO_OS_LINUX
				  ioService,
#endif
				  fileOffset)
			, m_impl(fileHandle, buffer, byteCount)
		{}

	private:
		friend cppcoro::detail::io_operation<file_read_operation>;

		bool try_start() noexcept { return m_impl.try_start(*this); }

		file_read_operation_impl m_impl;
	};

	class file_read_operation_cancellable
		: public cppcoro::detail::io_operation_cancellable<file_read_operation_cancellable>
	{
	public:
		file_read_operation_cancellable(
#if CPPCORO_OS_LINUX
			io_service& ioService,
#endif
			detail::handle_t fileHandle,
			std::uint64_t fileOffset,
			void* buffer,
			std::size_t byteCount,
			cancellation_token&& cancellationToken) noexcept
			: cppcoro::detail::io_operation_cancellable<file_read_operation_cancellable>(
#if CPPCORO_OS_LINUX
				  ioService,
#endif
				  fileOffset,
				  std::move(cancellationToken))
			, m_impl(fileHandle, buffer, byteCount)
		{}

	private:
		friend cppcoro::detail::io_operation_cancellable<file_read_operation_cancellable>;

		bool try_start() noexcept { return m_impl.try_start(*this); }

		void cancel() noexcept { m_impl.cancel(*this); }

		file_read_operation_impl m_impl;
	};

}  // namespace cppcoro

#endif
