#pragma once
#define ASIO_STANDALONE

#include <functional>
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "data_def/proto/sim_data.pb.h"
#include "data_def/sim_vars.h"

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
  absl::Status Broadcast(const std::string& payload)
      LOCKS_EXCLUDED(connections_lock_);
  // Loop that processes incoming connections and messages.
  void ProcessEvents();

 private:
  void OnMessage(websocketpp::connection_hdl connection,
                 Server::message_ptr message);
  void OnOpen(websocketpp::connection_hdl connection);
  void OnClose(websocketpp::connection_hdl connection);

  void HandleMessage(Server::message_ptr message);

  void PushNewEvent(const WSEvent& evt) LOCKS_EXCLUDED(events_lock_);

  // The WS server.
  Server server_;
  std::queue<WSEvent> events_ GUARDED_BY(events_lock_);
  // Stores all connections.
  ConnectionSet connections_ GUARDED_BY(connections_lock_);
  // Mutex lock for connections_
  absl::Mutex connections_lock_;
  absl::Mutex events_lock_;
};

class SimDataBroadcaster {
 public:
  SimDataBroadcaster(std::unique_ptr<WebSocketServer> server,
                     const data::SimVars* const sim_vars)
      : server_(std::move(server)), sim_vars_(sim_vars){};

  void Run(absl::Duration delay = absl::Milliseconds(10));

 private:
  // Converts simvar struct into SimData proto.
  SimData ConvertSimData();
  SimData sim_data_;
  std::unique_ptr<WebSocketServer> server_;
  const data::SimVars* const sim_vars_;
};

}  // namespace ws
}  // namespace flight_panel
