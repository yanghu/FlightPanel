#include "WebSocketServer.h"

#include <memory>

#include "absl/synchronization/mutex.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace ws {
namespace {
void SendMsg(Server* server, websocketpp::connection_hdl connection,
             Server::message_ptr message) {
  try {
    server->send(connection, message->get_payload(), message->get_opcode());
  } catch (websocketpp::exception const& e) {
    SPDLOG_ERROR("Send message failed because: {} ", e.what());
  }
}
}  // namespace
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

WebSocketServer::WebSocketServer() {
  server_.init_asio();
  server_.set_error_channels(websocketpp::log::elevel::all);
  server_.set_access_channels(websocketpp::log::alevel::all ^
                              websocketpp::log::alevel::frame_payload);
  server_.set_message_handler(
      std::bind(&WebSocketServer::OnMessage, this, _1, _2));
  server_.set_open_handler(std::bind(&WebSocketServer::OnOpen, this, _1));
  server_.set_close_handler(std::bind(&WebSocketServer::OnClose, this, _1));
}

void WebSocketServer::PushNewEvent(const WSEvent& event) {
  absl::MutexLock l(&events_lock_);
  events_.push(WSEvent{event});

}

void WebSocketServer::OnMessage(websocketpp::connection_hdl connection,
                                Server::message_ptr message) {
  PushNewEvent(WSEvent{EventType::MESSAGE, connection, message});
  SPDLOG_INFO("OnMessage: {}", message->get_payload());
}

void WebSocketServer::OnOpen(websocketpp::connection_hdl connection) {
  PushNewEvent(WSEvent{EventType::SUBSCRIBE, connection});
  SPDLOG_INFO("OnOpen: new connection pushed");
}


void WebSocketServer::OnClose(websocketpp::connection_hdl connection) {
  PushNewEvent(WSEvent{EventType::UNSUBSCRIBE, connection});
  SPDLOG_INFO("OnClose: UNSUBSCRIBE pushed");
}

void WebSocketServer::ProcessEvents() {
  auto events_not_empty = [this] { return !events_.empty(); };
  while (true) {
    events_lock_.Lock();
    // Wait until there're new events.
    events_lock_.Await(absl::Condition(&events_not_empty));
    WSEvent event = events_.front();
    events_.pop();
    events_lock_.Unlock();

    switch (event.event_type) {
      case EventType::SUBSCRIBE: {
        absl::MutexLock l(&connections_lock_);
        connections_.insert(event.connection);
        SPDLOG_INFO("New connection added. Connection count: {}",
                     connections_.size());
        break;
      }
      case EventType::UNSUBSCRIBE: {
        absl::MutexLock l(&connections_lock_);
        connections_.erase(event.connection);
        SPDLOG_INFO("Connection closed. Connection count: {}",
                     connections_.size());
        break;
      }
      case EventType::MESSAGE: {
        HandleMessage(event.message);
        break;
      }
    }
  }
}

void WebSocketServer::HandleMessage(Server::message_ptr message) {
  SPDLOG_INFO("Message Received: {}", message->get_payload());
}

void WebSocketServer::Run(uint16_t port) {
  server_.listen(port);
  server_.start_accept();
  server_.run();
}
}  // namespace ws
}  // namespace flight_panel
