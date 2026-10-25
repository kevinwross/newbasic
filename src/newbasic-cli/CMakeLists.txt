add_executable(newbasic-cli main.cpp)
# Plan: binary name is newbasic.exe on Windows and newbasic on POSIX.
set_target_properties(newbasic-cli PROPERTIES OUTPUT_NAME newbasic)
newbasic_set_binary_output(newbasic-cli)
target_link_libraries(newbasic-cli PRIVATE newbasic-window newbasic-screen newbasic-core)
newbasic_target_warnings(newbasic-cli)
