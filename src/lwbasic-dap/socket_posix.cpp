#include "socket.h"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace lwbasic::dap {

Socket::~Socket() {
  Close();
}

Socket::Socket(Socket&& other) noexcept : handle_(other.handle_) {
  other.handle_ = kInvalid;
}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    Close();
    handle_ = other.handle_;
    other.handle_ = kInvalid;
  }
  return *this;
}

Socket Socket::ListenLoopback(int port, std::string* error) {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    if (error != nullptr)
      *error = "could not create a socket";
    return Socket();
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(static_cast<uint16_t>(port));
  if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    if (error != nullptr)
      *error = "could not bind port " + std::to_string(port);
    ::close(fd);
    return Socket();
  }
  if (::listen(fd, 1) != 0) {
    if (error != nullptr)
      *error = "could not listen";
    ::close(fd);
    return Socket();
  }
  return Socket(fd);
}

Socket Socket::Accept(std::string* error) const {
  for (;;) {
    const int fd = ::accept(static_cast<int>(handle_), nullptr, nullptr);
    if (fd >= 0)
      return Socket(fd);
    if (errno == EINTR)
      continue;
    if (error != nullptr)
      *error = "could not accept a connection";
    return Socket();
  }
}

std::ptrdiff_t Socket::Recv(char* data, std::size_t size) {
  for (;;) {
    const ssize_t count = ::recv(static_cast<int>(handle_), data, size, 0);
    if (count >= 0)
      return static_cast<std::ptrdiff_t>(count);
    if (errno == EINTR)
      continue;
    return -1;
  }
}

std::ptrdiff_t Socket::Send(const char* data, std::size_t size) {
  for (;;) {
    // MSG_NOSIGNAL keeps a disconnected editor from killing the server with
    // SIGPIPE.
    const ssize_t count = ::send(static_cast<int>(handle_), data, size, MSG_NOSIGNAL);
    if (count >= 0)
      return static_cast<std::ptrdiff_t>(count);
    if (errno == EINTR)
      continue;
    return -1;
  }
}

int Socket::BoundPort() const {
  sockaddr_in bound{};
  socklen_t length = sizeof(bound);
  if (::getsockname(static_cast<int>(handle_), reinterpret_cast<sockaddr*>(&bound), &length) != 0)
    return -1;
  return ntohs(bound.sin_port);
}

void Socket::Close() {
  if (handle_ != kInvalid) {
    ::close(static_cast<int>(handle_));
    handle_ = kInvalid;
  }
}

} // namespace lwbasic::dap
