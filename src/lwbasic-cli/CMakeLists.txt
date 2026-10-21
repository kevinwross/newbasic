add_executable(lwbasic-cli main.cpp)
# Plan: binary name is lwbasic.exe
set_target_properties(lwbasic-cli PROPERTIES OUTPUT_NAME lwbasic)
target_link_libraries(lwbasic-cli PRIVATE lwbasic-screen lwbasic-core)
if(MSVC)
  target_compile_options(lwbasic-cli PRIVATE /WX)
endif()
