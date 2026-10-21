add_library(newbasic-debug STATIC
  src/debugger.cpp
)
target_include_directories(newbasic-debug PUBLIC include)
target_link_libraries(newbasic-debug PUBLIC newbasic-core)
newbasic_target_warnings(newbasic-debug)
