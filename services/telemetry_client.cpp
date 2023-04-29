#include "services/telemetry_client.h"

#include <memory>

#pragma warning(push)
#pragma warning(disable : 4702)
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#pragma warning(pop)

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;

namespace {

beast::error_code Run(std::string const& host,
                      std::string const& port,
                      std::string const& target,
                      int version,
                      net::io_context& ioc,
                      net::yield_context yield) {
  beast::error_code ec;

  // These objects perform our I/O
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);

  // Look up the domain name
  auto const results = resolver.async_resolve(host, port, yield[ec]);
  if (ec)
    return ec;

  // Set the timeout.
  stream.expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  stream.async_connect(results, yield[ec]);
  if (ec)
    return ec;

  // Set up an HTTP GET request message
  http::request<http::string_body> req{http::verb::get, target, version};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // Set the timeout.
  stream.expires_after(std::chrono::seconds(30));

  // Send the HTTP request to the remote host
  http::async_write(stream, req, yield[ec]);
  if (ec)
    return ec;

  // This buffer is used for reading and must be persisted
  beast::flat_buffer b;

  // Declare a container to hold the response
  http::response<http::dynamic_body> res;

  // Receive the HTTP response
  http::async_read(stream, b, res, yield[ec]);
  if (ec)
    return ec;

  // Write the message to standard out
  // std::cout << res << std::endl;

  // Gracefully close the socket
  stream.socket().shutdown(tcp::socket::shutdown_both, ec);

  // not_connected happens sometimes
  // so don't bother reporting it.
  //
  if (ec && ec != beast::errc::not_connected)
    return ec;

  // If we get here then the connection is closed gracefully

  return {};
}

}  // namespace

void TelemetryClient::Init() {
  std::string host;
  std::string port;
  std::string target;
  int version = 1;

  // Launch the asynchronous operation
  boost::asio::spawn(
      ioc_,
      [&, &ioc = ioc_](net::yield_context yield) {
        Run(host, port, target, version, ioc, yield);
      },
      // on completion, spawn will call this function
      [](std::exception_ptr ex) {
        // if an exception occurred in the coroutine,
        // it's something critical, e.g. out of memory
        // we capture normal errors in the ec
        // so we just rethrow the exception here,
        // which will cause `ioc.run()` to throw
        if (ex)
          std::rethrow_exception(ex);
      });
}

void TelemetryClient::Send(const TelemetryEvent& event) {}
