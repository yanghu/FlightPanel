#pragma once
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>
namespace flight_panel {

typedef websocketpp::server<websocketpp::config::asio> server;

// A websocket server to broadcast Sim data.
class WebSocketServer {
 public:
  WebSocketServer();

  void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

  void run(uint16_t port);

 private:
  server m_server;
};
}  // namespace flight_panel
