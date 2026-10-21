add_executable(newbasic-dap main.cpp)
if(WIN32)
  target_sources(newbasic-dap PRIVATE socket_win32.cpp)
  target_link_libraries(newbasic-dap PRIVATE ws2_32)
else()
  target_sources(newbasic-dap PRIVATE socket_posix.cpp)
endif()
# The DAP server drives the debugger, so it links the debug library (which carries the
# debugger and the protocol headers) as well as the core. It deliberately does not link
# the window library: a debug adapter has no display.
target_link_libraries(newbasic-dap PRIVATE newbasic-debug newbasic-screen newbasic-core)
newbasic_target_warnings(newbasic-dap)
