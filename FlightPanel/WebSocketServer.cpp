#include "WebSocketServer.h"

namespace flight_panel{

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

WebSocketServer::WebSocketServer() {
  m_server.init_asio();
  m_server.set_error_channels(websocketpp::log::elevel::all);
  m_server.set_access_channels(websocketpp::log::alevel::all ^
                               websocketpp::log::alevel::frame_payload);
  m_server.set_message_handler(std::bind(&WebSocketServer::on_message, this, _1, _2));
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl,
                                 server::message_ptr msg) {
  try {
    m_server.send(hdl, msg->get_payload(), msg->get_opcode());
  } catch (websocketpp::exception const& e) {
    std::cout << "Echo failed because: "
              << "(" << e.what() << ")" << std::endl;
  }
}

void WebSocketServer::run(uint16_t port) { 
  m_server.listen(port);
  m_server.start_accept();
  m_server.run();
}
}  // namespace
