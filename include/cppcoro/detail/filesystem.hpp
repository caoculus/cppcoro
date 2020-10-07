///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_FILESYSTEM_HPP_INCLUDED
#define CPPCORO_FILESYSTEM_HPP_INCLUDED

#include <cppcoro/config.hpp>

#if CPPCORO_FILESYSTEM_STANDARD
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

namespace cppcoro
{
#if CPPCORO_FILESYSTEM_STANDARD
	namespace fs = std::filesystem;
#else
	namespace fs = std::experimental::filesystem;
#endif
}  // namespace cppcoro

#endif  // CPPCORO_FILESYSTEM_HPP_INCLUDED
