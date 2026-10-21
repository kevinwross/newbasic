add_executable(lwbasic-cli main.cpp)
# Plan: binary name is lwbasic.exe on Windows and lwbasic on POSIX.
set_target_properties(lwbasic-cli PROPERTIES OUTPUT_NAME lwbasic)
target_link_libraries(lwbasic-cli PRIVATE lwbasic-window lwbasic-screen lwbasic-core)
if(MSVC)
  target_compile_options(lwbasic-cli PRIVATE /WX)
endif()
