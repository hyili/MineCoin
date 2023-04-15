//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, asynchronous
//
//------------------------------------------------------------------------------

#include "root_certificates.hpp"
#include "utils.hpp"

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

bool is_running = false;

void sig_handle(int sig) {
  is_running = false;
  std::cout << " [SigHandle] " << sig << " received, terminating ... "
            << std::endl;
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<
        beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;
    std::string access_key;
    std::string secret_key;
    std::string algo = "sha256";
    std::string nonce;
    std::string signature;
    json::value auth_json;
    std::string auth_text;

  public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc, ssl::context& ctx)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc), ctx)
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));

        // load keys
        load_keys(access_key, secret_key);
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
      if (ec)
        return fail(ec, "resolve");

      // Set a timeout on the operation
      beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

      // Make the connection on the IP address we get from a lookup
      beast::get_lowest_layer(ws_).async_connect(
          results,
          beast::bind_front_handler(&session::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type ep) {
      if (ec)
        return fail(ec, "connect");

      // Set a timeout on the operation
      beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

      // Set SNI Hostname (many hosts need this to handshake successfully)
      if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(),
                                    host_.c_str())) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                               net::error::get_ssl_category());
        return fail(ec, "connect");
      }

      // Update the host_ string. This will provide the value of the
      // Host HTTP header during the WebSocket handshake.
      // See https://tools.ietf.org/html/rfc7230#section-5.4
      host_ += ':' + std::to_string(ep.port());

      // Perform the SSL handshake
      ws_.next_layer().async_handshake(
          ssl::stream_base::client,
          beast::bind_front_handler(&session::on_ssl_handshake,
                                    shared_from_this()));
    }

    void on_ssl_handshake(beast::error_code ec) {
      if (ec)
        return fail(ec, "ssl_handshake");

      // Turn off the timeout on the tcp_stream, because
      // the websocket stream has its own timeout system.
      beast::get_lowest_layer(ws_).expires_never();

      websocket::stream_base::timeout timeout_opt{
          std::chrono::seconds(30), std::chrono::seconds(5), false};
      ws_.set_option(timeout_opt);

      // Set suggested timeout settings for the websocket
      //      ws_.set_option(
      //          websocket::stream_base::timeout::suggested(beast::role_type::client));

      // Set a decorator to change the User-Agent of the handshake
      ws_.set_option(
          websocket::stream_base::decorator([](websocket::request_type &req) {
            req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
          }));

      // Perform the websocket handshake
      ws_.async_handshake(host_, "/ws",
                          beast::bind_front_handler(&session::on_handshake,
                                                    shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
      if (ec)
        return fail(ec, "handshake");

      // json payload
      uint64_t epoch = duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
      nonce = std::to_string(epoch);
      signature = hmac(algo, secret_key, nonce);
      auth_json = {{"action", "auth"},  {"apiKey", access_key},
                   {"nonce", epoch},    {"signature", signature},
                   {"id", "client-id"}, {"filters", {"account"}}};
      auth_text = json::serialize(auth_json);
      std::cout << " [Preview] " << auth_text << std::endl;

      // Send the message
      std::cout << " [OnHandShake] Sending message ..." << std::endl;
      ws_.async_write(
          net::buffer(auth_text),
          // net::buffer(text_),
          beast::bind_front_handler(&session::on_write, shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Read a message into our buffer
        std::cout << " [OnWrite] Reading message into buffer ..." << std::endl;
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Print
        std::cout << " [OnRead] Print from buffer ..." << std::endl;
        std::cout << " [Message] " << beast::make_printable(buffer_.data())
                  << std::endl;

        buffer_.clear();

        if (is_running) {
          // Read a message into our buffer
          std::cout << " [OnWrite] Reading message into buffer ..."
                    << std::endl;
          ws_.async_read(buffer_, beast::bind_front_handler(
                                      &session::on_read, shared_from_this()));
        }
    }

    void on_close(beast::error_code ec) {
      if (ec)
        return fail(ec, "close");

      // If we get here then the connection is closed gracefully

      // The make_printable() function helps print a ConstBufferSequence
      std::cout << " [OnClose] Print from buffer ..." << std::endl;
      std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }

    // call from main program
    void call_close() {
      // Close the WebSocket connection
      std::cout << " [OnRead] Closing websocket ..." << std::endl;
      ws_.async_close(
          websocket::close_code::normal,
          beast::bind_front_handler(&session::on_close, shared_from_this()));
    }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
  signal(SIGTERM, sig_handle);
  signal(SIGINT, sig_handle);

  // Check command line arguments.
  if (argc != 4) {
    std::cerr << "Usage: websocket-client-async-ssl <host> <port> <text>\n"
              << "Example:\n"
              << "    websocket-client-async-ssl echo.websocket.org 443 "
                 "\"Hello, world!\"\n";
    return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const text = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Launch the asynchronous operation
    auto ptr = std::make_shared<session>(ioc, ctx);
    ptr->run(host, port, text);

    // Run the I/O service. The call will return when
    // the socket is closed.
    is_running = true;
    ioc.run();

    while (is_running) {
      sleep(1);
    };
    ioc.stop();
    ptr->call_close();

    return EXIT_SUCCESS;
}
