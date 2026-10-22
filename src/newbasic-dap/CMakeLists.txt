add_executable(newbasic-dap main.cpp)
if(WIN32)
  target_sources(newbasic-dap PRIVATE socket_win32.cpp)
  target_link_libraries(newbasic-dap PRIVATE ws2_32)
else()
  target_sources(newbasic-dap PRIVATE socket_posix.cpp)
endif()
# The DAP server drives the debugger, so it links the debug library (which carries the
# debugger and the protocol headers) as well as the core. It links the window library too:
# a debug session shows the program's GUI output by default, with --nowindow selecting a
# headless screen for CI and other non-interactive clients.
target_link_libraries(newbasic-dap PRIVATE newbasic-debug newbasic-window newbasic-screen
                                         newbasic-core)
newbasic_target_warnings(newbasic-dap)
