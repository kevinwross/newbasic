add_executable(newbasic-tests
  test_number_format.cpp
  test_line_store.cpp
  test_bas_text.cpp
  test_lexer.cpp
  test_names.cpp
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
  test_samples.cpp
)
target_include_directories(newbasic-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(newbasic-tests PRIVATE
  newbasic-core
  newbasic-debug
  newbasic-screen
  newbasic-window
  Catch2::Catch2WithMain
  rapidcheck-vendored
)
target_compile_definitions(newbasic-tests PRIVATE
  NEWBASIC_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../fixtures"
  NEWBASIC_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../samples"
)
if(MSVC)
  target_compile_options(newbasic-tests PRIVATE /WX)
endif()

add_test(NAME newbasic-tests COMMAND newbasic-tests)

# Shipped samples are graphics regressions: each runs headlessly with a scripted
# ESC and the test checks the pixels it painted in SCREEN 720.
add_test(NAME sample-neon-horizon COMMAND newbasic-tests "[neon]")
add_test(NAME sample-ringworld COMMAND newbasic-tests "[ringworld]")
add_test(NAME sample-fireworks COMMAND newbasic-tests "[fireworks]")
add_test(NAME sample-spirograph COMMAND newbasic-tests "[spirograph]")
set_tests_properties(sample-neon-horizon sample-ringworld sample-fireworks
                     sample-spirograph PROPERTIES TIMEOUT 60)

# Regression for the CLI program-file argument: a Windows-style path must be
# translated at the host boundary on POSIX. --gpu-shot also gives the CLI a
# headless fallback when no terminal or window is available.
set(dos_fixture "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/m0/hello.bas")
string(REPLACE "/" "\\\\" dos_fixture "${dos_fixture}")
add_test(
  NAME cli-dos-path
  COMMAND $<TARGET_FILE:newbasic-cli>
          --gpu-shot "${CMAKE_CURRENT_BINARY_DIR}/cli-dos-path.ppm"
          "${dos_fixture}"
)
set_tests_properties(cli-dos-path PROPERTIES TIMEOUT 60)

# Covers the newbasic-dap *process* boundary (framing over stdin/stdout), which
# the in-process Catch2 tests cannot reach.
add_test(
  NAME dap-smoke
  COMMAND ${CMAKE_COMMAND}
          "-DDAP_EXE=$<TARGET_FILE:newbasic-dap>"
          "-DTMP_DIR=${CMAKE_CURRENT_BINARY_DIR}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/dap_smoke.cmake"
)
set_tests_properties(dap-smoke PROPERTIES TIMEOUT 60)

