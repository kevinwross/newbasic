add_executable(lwbasic-tests
  test_number_format.cpp
  test_line_store.cpp
  test_bas_text.cpp
  test_lexer.cpp
  test_expressions.cpp
  test_statements.cpp
  test_arrays.cpp
  test_data.cpp
  test_userfuncs.cpp
  test_errors.cpp
  test_files.cpp
  test_fileio.cpp
  test_events.cpp
  test_dap_framing.cpp
  test_dap_json.cpp
  test_dap_handler.cpp
  test_dap_session.cpp
  test_fstok_tokens.cpp
  test_fstok_codec.cpp
  test_fstok_properties.cpp
  test_robustness.cpp
  test_debugger.cpp
  test_repl.cpp
  test_vm_print.cpp
  test_coverage.cpp
  test_using.cpp
  test_text_window.cpp
  test_key.cpp
  test_field_conv.cpp
  test_random_files.cpp
  test_program_control.cpp
  test_chaining.cpp
  test_key_trap.cpp
  test_inventory.cpp
  test_renum.cpp
  test_printer.cpp
  test_graphics.cpp
  test_sound.cpp
  test_text_render.cpp
  test_properties.cpp
)
target_include_directories(lwbasic-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lwbasic-tests PRIVATE
  lwbasic-core
  lwbasic-debug
  lwbasic-screen
  Catch2::Catch2WithMain
  rapidcheck-vendored
)
target_compile_definitions(lwbasic-tests PRIVATE
  LWBASIC_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../fixtures"
)
if(MSVC)
  target_compile_options(lwbasic-tests PRIVATE /WX)
endif()

add_test(NAME lwbasic-tests COMMAND lwbasic-tests)

# Covers the lwbasic-dap *process* boundary (framing over stdin/stdout), which
# the in-process Catch2 tests cannot reach.
add_test(
  NAME dap-smoke
  COMMAND ${CMAKE_COMMAND}
          "-DDAP_EXE=$<TARGET_FILE:lwbasic-dap>"
          "-DTMP_DIR=${CMAKE_CURRENT_BINARY_DIR}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/dap_smoke.cmake"
)
set_tests_properties(dap-smoke PROPERTIES TIMEOUT 60)

