#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace newbasic::dap {

// Minimal TCP socket used only by the DAP server's --port transport. The
// implementation is platform-specific; the protocol layer sees plain bytes.
class Socket {
public:
  Socket() = default;
  ~Socket();

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

  // Binds to 127.0.0.1:port and listens for one connection at a time. Port 0
  // picks a free port; BoundPort() reports the actual one.
  static Socket ListenLoopback(int port, std::string* error);

  // Blocks until a client connects. The returned socket is invalid on error.
  Socket Accept(std::string* error) const;

  std::ptrdiff_t Recv(char* data, std::size_t size);
  std::ptrdiff_t Send(const char* data, std::size_t size);

  int BoundPort() const;
  void Close();
  bool Valid() const { return handle_ != kInvalid; }

private:
  static constexpr std::intptr_t kInvalid = -1;
  explicit Socket(std::intptr_t handle) : handle_(handle) {}
  std::intptr_t handle_ = kInvalid;
};

} // namespace newbasic::dap
