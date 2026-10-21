#include "socket.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

namespace lwbasic::dap {
namespace {

bool EnsureWinsock() {
  static const bool ready = [] {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
  }();
  return ready;
}

} // namespace

Socket::~Socket() { Close(); }

Socket::Socket(Socket&& other) noexcept : handle_(other.handle_) { other.handle_ = kInvalid; }

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    Close();
    handle_ = other.handle_;
    other.handle_ = kInvalid;
  }
  return *this;
}

Socket Socket::ListenLoopback(int port, std::string* error) {
  if (!EnsureWinsock()) {
    if (error != nullptr)
      *error = "winsock initialisation failed";
    return Socket();
  }
  const SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == INVALID_SOCKET) {
    if (error != nullptr)
      *error = "could not create a socket";
    return Socket();
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(static_cast<u_short>(port));
  if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    if (error != nullptr)
      *error = "could not bind port " + std::to_string(port);
    ::closesocket(listener);
    return Socket();
  }
  if (::listen(listener, 1) != 0) {
    if (error != nullptr)
      *error = "could not listen";
    ::closesocket(listener);
    return Socket();
  }
  return Socket(static_cast<std::intptr_t>(listener));
}

Socket Socket::Accept(std::string* error) const {
  for (;;) {
    const SOCKET client = ::accept(static_cast<SOCKET>(handle_), nullptr, nullptr);
    if (client != INVALID_SOCKET)
      return Socket(static_cast<std::intptr_t>(client));
    if (WSAGetLastError() == WSAEINTR)
      continue;
    if (error != nullptr)
      *error = "could not accept a connection";
    return Socket();
  }
}

std::ptrdiff_t Socket::Recv(char* data, std::size_t size) {
  for (;;) {
    const int count = ::recv(static_cast<SOCKET>(handle_), data, static_cast<int>(size), 0);
    if (count >= 0)
      return static_cast<std::ptrdiff_t>(count);
    if (WSAGetLastError() == WSAEINTR)
      continue;
    return -1;
  }
}

std::ptrdiff_t Socket::Send(const char* data, std::size_t size) {
  for (;;) {
    const int count = ::send(static_cast<SOCKET>(handle_), data, static_cast<int>(size), 0);
    if (count >= 0)
      return static_cast<std::ptrdiff_t>(count);
    if (WSAGetLastError() == WSAEINTR)
      continue;
    return -1;
  }
}

int Socket::BoundPort() const {
  sockaddr_in bound{};
  int length = static_cast<int>(sizeof(bound));
  if (::getsockname(static_cast<SOCKET>(handle_), reinterpret_cast<sockaddr*>(&bound), &length) != 0)
    return -1;
  return ntohs(bound.sin_port);
}

void Socket::Close() {
  if (handle_ != kInvalid) {
    ::closesocket(static_cast<SOCKET>(handle_));
    handle_ = kInvalid;
  }
}

} // namespace lwbasic::dap
