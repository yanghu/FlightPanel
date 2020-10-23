#pragma once
#define ASIO_STANDALONE

#include <functional>
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"

namespace flight_panel {
namespace ws {

using ConnectionSet = std::set<websocketpp::connection_hdl,
                               std::owner_less<websocketpp::connection_hdl>>;

using Server = websocketpp::server<websocketpp::config::asio>;

enum class EventType {
  SUBSCRIBE,
  UNSUBSCRIBE,
  MESSAGE,
};

// Websocket events, either open/close connections, or a new message.
struct WSEvent {
  WSEvent(EventType t, websocketpp::connection_hdl h)
      : event_type(t), connection(h){};
  WSEvent(EventType t, websocketpp::connection_hdl h, Server::message_ptr m)
      : event_type(t), connection(h), message(m){};

  EventType event_type;
  websocketpp::connection_hdl connection;
  Server::message_ptr message;
};

// A websocket server to broadcast Sim data.
class WebSocketServer {
 public:
  WebSocketServer();

  // Listen to port and run the Ws server
  void Run(uint16_t port);
  // Loop that processes incoming connections and messages.
  void ProcessEvents();

 private:
  void OnMessage(websocketpp::connection_hdl connection,
                 Server::message_ptr message);
  void OnOpen(websocketpp::connection_hdl connection);
  void OnClose(websocketpp::connection_hdl connection);

  void HandleMessage(Server::message_ptr message);

  void PushNewEvent(const WSEvent& evt) LOCKS_EXCLUDED(events_);

  // The WS server.
  Server server_;
  std::queue<WSEvent> events_ GUARDED_BY(events_lock_);
  // Stores all connections.
  ConnectionSet connections_ GUARDED_BY(connections_lock_);
  // Mutex lock for connections_
  absl::Mutex connections_lock_;
  absl::Mutex events_lock_;
};
}  // namespace ws
}  // namespace flight_panel
