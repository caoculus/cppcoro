///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_COROUTINE_HPP_INCLUDED
#define CPPCORO_COROUTINE_HPP_INCLUDED

#include <cppcoro/config.hpp>

#if CPPCORO_COROUTINE_STANDARD
#include <coroutine>
#else
#include <experimental/coroutine>
#endif

namespace cppcoro
{
#if CPPCORO_COROUTINE_STANDARD
	namespace coro = std;
#else
	namespace coro = std::experimental;
#endif
}  // namespace cppcoro

#endif  // CPPCORO_COROUTINE_HPP_INCLUDED
