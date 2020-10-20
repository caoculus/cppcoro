list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

include(CMakeFindDependencyMacro)
find_dependency(CppcoroCoroutines QUIET REQUIRED)

if(@CPPCORO_USE_IO_RING@)
  find_dependency(liburing QUIET REQUIRED)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cppcoroTargets.cmake")
