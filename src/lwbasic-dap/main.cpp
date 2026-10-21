// lwbasic-dap: a Debug Adapter Protocol server (plan section 5). The protocol itself lives in
// lwbasic::dap (framing, JSON, dispatcher, session) and is unit-tested without stdio; this
// executable is only the process boundary.
//
// Two transports, because editors differ:
//
//   (default)      DAP over stdin/stdout. An adapter that spawns the process - nvim-dap,
//                  Emacs dap-mode, or a VS Code extension - drives this.
//   --port N       DAP over a TCP socket on 127.0.0.1, for editors that want a debug server,
//                  such as VS Code's "debugServer": N. Port 0 picks a free port and reports it
//                  on stderr, which is what makes the transport testable.
//
// The listener is bound to the loopback interface only: a debug port gives whoever connects the
// ability to run and inspect programs, so it is not offered as a network service.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include <utility>

#include "lwbasic/dap/session.h"
#include "lwbasic/debugger.h"
#include "lwbasic/host.h"
#include "lwbasic/screen/console_screen.h"
#include "lwbasic/vm.h"
#include "socket.h"

namespace {

// A std::streambuf over a connected socket. This is the whole of the TCP transport: Session
// speaks istream/ostream, so pointing those at the socket is all it takes, and the protocol
// layer stays untouched and unit-tested.
class SocketBuf : public std::streambuf {
public:
  explicit SocketBuf(lwbasic::dap::Socket socket) : socket_(std::move(socket)) {}
  SocketBuf(const SocketBuf&) = delete;
  SocketBuf& operator=(const SocketBuf&) = delete;
  ~SocketBuf() override = default;

protected:
  int_type underflow() override {
    const std::ptrdiff_t received = socket_.Recv(buffer_, sizeof(buffer_));
    if (received <= 0)
      return traits_type::eof();
    setg(buffer_, buffer_, buffer_ + received);
    return traits_type::to_int_type(*gptr());
  }

  int_type overflow(int_type ch) override {
    if (traits_type::eq_int_type(ch, traits_type::eof()))
      return traits_type::not_eof(ch);
    const char byte = static_cast<char>(ch);
    return socket_.Send(&byte, 1) == 1 ? ch : traits_type::eof();
  }

  std::streamsize xsputn(const char* data, std::streamsize count) override {
    std::streamsize sent = 0;
    while (sent < count) {
      const std::ptrdiff_t result =
          socket_.Send(data + sent, static_cast<std::size_t>(count - sent));
      if (result <= 0)
        return sent;
      sent += result;
    }
    return sent;
  }

private:
  lwbasic::dap::Socket socket_;
  char buffer_[4096];
};

void PrintUsage() {
  std::fprintf(stderr,
               "lwbasic-dap: a Debug Adapter Protocol server.\n"
               "\n"
               "  lwbasic-dap            Speak DAP on stdin and stdout.\n"
               "  lwbasic-dap --port N   Listen for DAP on 127.0.0.1:N instead. Port 0 picks a\n"
               "                         free port and reports it. Use this for editors that want\n"
               "                         a debug server, such as VS Code debugServer.\n");
}

// Serves DAP on a listening socket until the listener fails. Each connection gets a fresh session
// over the same machine, so an editor can reconnect after a reload.
int RunPort(int port) {
  std::string error;
  lwbasic::dap::Socket listener = lwbasic::dap::Socket::ListenLoopback(port, &error);
  if (!listener.Valid()) {
    std::fprintf(stderr, "lwbasic-dap: %s\n", error.c_str());
    return 1;
  }

  // Report the port actually bound, which is what makes --port 0 usable by a test.
  std::fprintf(stderr, "lwbasic-dap: listening on 127.0.0.1:%d\n", listener.BoundPort());
  std::fflush(stderr);

  for (;;) {
    lwbasic::dap::Socket client = listener.Accept(&error);
    if (!client.Valid())
      break;
    {
      lwbasic::screen::ConsoleScreen screen;
      lwbasic::SystemHost host;
      lwbasic::Vm vm(screen, &host);
      lwbasic::debug::Debugger debugger(vm);
      lwbasic::dap::Session session(vm, debugger);
      SocketBuf buffer(std::move(client)); // closes the socket when it goes out of scope
      std::istream in(&buffer);
      std::ostream out(&buffer);
      session.Run(in, out);
      out.flush();
    }
  }

  return 0;
}

int RunStdio() {
  lwbasic::screen::ConsoleScreen screen;
  lwbasic::SystemHost host;
  lwbasic::Vm vm(screen, &host);
  lwbasic::debug::Debugger debugger(vm);
  lwbasic::dap::Session session(vm, debugger);
  session.Run(std::cin, std::cout);
  std::cout.flush();
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  int port = -1;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-?") == 0) {
      PrintUsage();
      return 0;
    }
    if (std::strcmp(argv[i], "--port") == 0) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "lwbasic-dap: --port needs a number\n");
        return 1;
      }
      port = std::atoi(argv[++i]);
      if (port < 0 || port > 65535) {
        std::fprintf(stderr, "lwbasic-dap: --port must be 0 to 65535\n");
        return 1;
      }
      continue;
    }
    std::fprintf(stderr, "lwbasic-dap: unknown argument: %s\n", argv[i]);
    PrintUsage();
    return 1;
  }
  if (port >= 0)
    return RunPort(port);
  return RunStdio();
}
