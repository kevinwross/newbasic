#pragma once

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>

namespace lwbasic::dap {

// Debug Adapter Protocol base protocol: every message is
//
//   Content-Length: <n>\r\n
//   \r\n
//   <n bytes of UTF-8 JSON>
//
// (https://microsoft.github.io/debug-adapter-protocol/specification). This
// header implements only the transport framing; the JSON layer and the
// request handling live above it, so both stay unit-testable without a
// running client.
struct Frame {
  bool ok = false;   // true when a complete frame was read
  std::string body;  // JSON text
  std::string error; // reason when !ok ("eof", "missing Content-Length", ...)
};

// Reads one framed message. At end of input returns {ok=false, error="eof"}.
inline Frame ReadFrame(std::istream& in) {
  Frame frame;
  std::size_t length = 0;
  bool have_length = false;
  bool saw_any_line = false;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      break; // end of headers
    saw_any_line = true;
    const std::string key = "Content-Length:";
    if (line.compare(0, key.size(), key) != 0)
      continue; // ignore other headers
    std::size_t index = key.size();
    while (index < line.size() && (line[index] == ' ' || line[index] == '\t'))
      ++index;
    bool digits = false;
    length = 0;
    for (; index < line.size(); ++index) {
      if (line[index] < '0' || line[index] > '9')
        break;
      length = length * 10 + static_cast<std::size_t>(line[index] - '0');
      digits = true;
    }
    have_length = digits;
  }
  if (!have_length) {
    frame.error = saw_any_line ? "missing Content-Length" : "eof";
    return frame;
  }
  frame.body.resize(length);
  if (length > 0) {
    in.read(frame.body.data(), static_cast<std::streamsize>(length));
    if (static_cast<std::size_t>(in.gcount()) != length) {
      frame.error = "truncated body";
      return frame;
    }
  }
  frame.ok = true;
  return frame;
}

// Writes one framed message and flushes it (a debugger client waits).
inline void WriteFrame(std::ostream& out, const std::string& body) {
  out << "Content-Length: " << body.size() << "\r\n\r\n" << body;
  out.flush();
}

} // namespace lwbasic::dap
