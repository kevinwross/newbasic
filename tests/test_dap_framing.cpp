// M5: Debug Adapter Protocol transport framing.
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include "lwbasic/dap/framing.h"

TEST_CASE("DAP framing: exact wire format and round trip", "[dap]") {
  std::stringstream stream;
  lwbasic::dap::WriteFrame(stream, "{\"seq\":1}");
  CHECK(stream.str() == "Content-Length: 9\r\n\r\n{\"seq\":1}");

  const lwbasic::dap::Frame frame = lwbasic::dap::ReadFrame(stream);
  CHECK(frame.ok);
  CHECK(frame.body == "{\"seq\":1}");
  CHECK(frame.error.empty());
}

TEST_CASE("DAP framing: several messages, including an empty body", "[dap]") {
  std::stringstream stream;
  lwbasic::dap::WriteFrame(stream, "{}");
  lwbasic::dap::WriteFrame(stream, "");
  lwbasic::dap::WriteFrame(stream, "second");

  CHECK(lwbasic::dap::ReadFrame(stream).body == "{}");
  const lwbasic::dap::Frame empty = lwbasic::dap::ReadFrame(stream);
  CHECK(empty.ok);
  CHECK(empty.body.empty());
  CHECK(lwbasic::dap::ReadFrame(stream).body == "second");

  const lwbasic::dap::Frame end = lwbasic::dap::ReadFrame(stream);
  CHECK_FALSE(end.ok);
  CHECK(end.error == "eof");
}

TEST_CASE("DAP framing: unknown headers and loose spacing are tolerated", "[dap]") {
  std::stringstream stream("Content-Type: application/vsdp\r\nContent-Length:   5\r\n\r\nhello");
  const lwbasic::dap::Frame frame = lwbasic::dap::ReadFrame(stream);
  CHECK(frame.ok);
  CHECK(frame.body == "hello");
}

TEST_CASE("DAP framing: errors are reported, not guessed", "[dap]") {
  std::stringstream missing("X-Test: 1\r\n\r\nbody");
  const lwbasic::dap::Frame no_length = lwbasic::dap::ReadFrame(missing);
  CHECK_FALSE(no_length.ok);
  CHECK(no_length.error == "missing Content-Length");

  std::stringstream truncated("Content-Length: 10\r\n\r\nshort");
  const lwbasic::dap::Frame cut = lwbasic::dap::ReadFrame(truncated);
  CHECK_FALSE(cut.ok);
  CHECK(cut.error == "truncated body");

  std::stringstream empty("");
  const lwbasic::dap::Frame eof = lwbasic::dap::ReadFrame(empty);
  CHECK_FALSE(eof.ok);
  CHECK(eof.error == "eof");
}

TEST_CASE("DAP framing: a body containing CRLF and NUL is preserved byte-exact", "[dap]") {
  const std::string body = "a\r\nb\0c";
  std::stringstream stream;
  lwbasic::dap::WriteFrame(stream, body);
  const lwbasic::dap::Frame frame = lwbasic::dap::ReadFrame(stream);
  REQUIRE(frame.ok);
  CHECK(frame.body.size() == body.size());
  CHECK(frame.body == body);
}
