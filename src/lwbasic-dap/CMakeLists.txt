add_executable(lwbasic-dap main.cpp)
# The DAP server drives the debugger, so it links the debug library (which carries the
# debugger and the protocol headers) as well as the core.
target_link_libraries(lwbasic-dap PRIVATE lwbasic-debug lwbasic-screen lwbasic-core ws2_32)
if(MSVC)
  target_compile_options(lwbasic-dap PRIVATE /WX)
endif()
