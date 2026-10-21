add_library(lwbasic-debug STATIC
  src/debugger.cpp
)
target_include_directories(lwbasic-debug PUBLIC include)
target_link_libraries(lwbasic-debug PUBLIC lwbasic-core)
if(MSVC)
  target_compile_options(lwbasic-debug PRIVATE /WX)
endif()
