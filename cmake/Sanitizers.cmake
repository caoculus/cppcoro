option(CPPCORO_ENABLE_ASAN "Enable address sanitizer" OFF)
option(CPPCORO_ENABLE_TSAN "Enable thread sanitizer" OFF)
option(CPPCORO_ENABLE_UBSAN "Enable undefined behavior sanitizer" OFF)

function(cppcoro_add_sanitizers __target __visibility)

  macro(_add_flags)
    target_compile_options(${__target} ${__visibility} ${ARGV})
    target_link_options(${__target} ${__visibility} ${ARGV})
  endmacro()

  if(CPPCORO_ENABLE_ASAN)
    _add_flags(-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer)
  endif()

  if(CPPCORO_ENABLE_TSAN)
    _add_flags(-fsanitize=thread -g -O1)
  endif()

  if(CPPCORO_ENABLE_UBSAN)
    _add_flags(-fsanitize=undefined)
  endif()
endfunction()
