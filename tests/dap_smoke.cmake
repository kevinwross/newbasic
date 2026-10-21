# Integration smoke test for lwbasic-dap (plan section 5): runs the real
# executable with a framed DAP request on stdin and checks the framed response.
# Driven by CMake rather than the Catch2 binary so the process boundary is
# covered in CI, not just the in-process protocol layer.
#
# Usage: cmake -DDAP_EXE=<path to lwbasic-dap> -P dap_smoke.cmake

if(NOT DEFINED DAP_EXE)
  message(FATAL_ERROR "DAP_EXE was not set")
endif()
if(NOT EXISTS "${DAP_EXE}")
  message(FATAL_ERROR "lwbasic-dap executable not found: ${DAP_EXE}")
endif()

# Scratch space: prefer the caller's build directory. Writing into the source
# tree (the old behaviour) fails on a read-only checkout and can race a
# concurrent run, so TMP_DIR is passed in by the CTest registration; the
# fallback only exists for a manual `cmake -P` invocation.
if(NOT DEFINED TMP_DIR)
  set(TMP_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()
if(NOT EXISTS "${TMP_DIR}")
  file(MAKE_DIRECTORY "${TMP_DIR}")
endif()

set(request "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}")
string(LENGTH "${request}" request_length)

set(request_file "${TMP_DIR}/dap_request.tmp")
file(WRITE "${request_file}" "Content-Length: ${request_length}\r\n\r\n${request}")

execute_process(
  COMMAND "${DAP_EXE}"
  INPUT_FILE "${request_file}"
  OUTPUT_VARIABLE response
  ERROR_VARIABLE errors
  RESULT_VARIABLE status
  TIMEOUT 30)

file(REMOVE "${request_file}")

if(NOT status EQUAL 0)
  message(FATAL_ERROR "lwbasic-dap exited with ${status}; stderr: ${errors}")
endif()

# The response must be framed and well formed. CMake's execute_process may
# normalise CRLF to LF on POSIX, so accept either line ending here while the
# producer still writes the protocol's required CRLF.
if(NOT response MATCHES "^Content-Length: [0-9]+\r?\n\r?\n")
  message(FATAL_ERROR "response is not framed: [${response}]")
endif()
if(NOT response MATCHES "\"type\":\"response\"")
  message(FATAL_ERROR "response has no type: [${response}]")
endif()
if(NOT response MATCHES "\"success\":true")
  message(FATAL_ERROR "initialize was not successful: [${response}]")
endif()
if(NOT response MATCHES "\"command\":\"initialize\"")
  message(FATAL_ERROR "response does not echo the command: [${response}]")
endif()
if(NOT response MATCHES "supportsConfigurationDoneRequest")
  message(FATAL_ERROR "response is missing the capability body: [${response}]")
endif()

# Two request/response turns in one process, to prove the loop keeps going.
set(two_requests "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}")
set(second "{\"seq\":2,\"type\":\"request\",\"command\":\"threads\"}")
string(LENGTH "${two_requests}" first_length)
string(LENGTH "${second}" second_length)
file(WRITE "${request_file}"
  "Content-Length: ${first_length}\r\n\r\n${two_requests}"
  "Content-Length: ${second_length}\r\n\r\n${second}")
execute_process(
  COMMAND "${DAP_EXE}"
  INPUT_FILE "${request_file}"
  OUTPUT_VARIABLE both
  ERROR_VARIABLE errors
  RESULT_VARIABLE status
  TIMEOUT 30)
file(REMOVE "${request_file}")

if(NOT status EQUAL 0)
  message(FATAL_ERROR "lwbasic-dap exited with ${status} on the second run; stderr: ${errors}")
endif()
if(NOT both MATCHES "\"command\":\"initialize\"")
  message(FATAL_ERROR "first response missing: [${both}]")
endif()
if(NOT both MATCHES "\"command\":\"threads\"")
  message(FATAL_ERROR "second response missing: [${both}]")
endif()
if(NOT both MATCHES "\"name\":\"main\"")
  message(FATAL_ERROR "threads response has no thread: [${both}]")
endif()

message(STATUS "lwbasic-dap smoke test passed")
