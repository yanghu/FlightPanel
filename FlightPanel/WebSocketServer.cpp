#include "WebSocketServer.h"

#include <memory>

#include "absl/synchronization/mutex.h"
#include "absl/random/random.h"
#include "absl/time/clock.h"
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

SimData FakeSimData() {
  SimData data;
  const int kMaxSpeed = 180;
  static double speed = 0;
  data.mutable_aircraft_info()->set_call_sign("AXSGS");
  absl::BitGen bitgen;
  speed += 0.4;
  if (speed > kMaxSpeed) {
    speed -= kMaxSpeed;
  }
  auto instruments = data.mutable_instruments();
  instruments->set_indicated_airspeed(speed);
  instruments->set_pitch_angle(int(speed)%30);
  instruments->set_bank_angle(int(speed)%60);
  instruments->set_kohlsman_setting_hg(29.92);
  instruments->set_indicated_altitude(int(speed*50)%9000);
  return data;

}
}  // namespace

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

WebSocketServer::WebSocketServer() {
  server_.init_asio();
  //server_.set_error_channels(websocketpp::log::elevel::all);
  //server_.set_access_channels(websocketpp::log::alevel::all ^
   //                           websocketpp::log::alevel::frame_payload);
  server_.clear_access_channels(websocketpp::log::alevel::frame_header |
                               websocketpp::log::alevel::frame_payload);
  // this will turn off console output for frame header and payload

  server_.clear_access_channels(websocketpp::log::alevel::all);
  // this will turn off everything in console output
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

void WebSocketServer::Broadcast(const std::string& payload) {
  // Construct the message
  {
    absl::MutexLock l(&connections_lock_);
    // SPDLOG_INFO("Broadcasting data: {}", payload);
    for (auto connection : connections_) {
      try {
        server_.send(connection, payload, websocketpp::frame::opcode::binary);
      } catch (websocketpp::exception const& e) {
        SPDLOG_ERROR("Send message failed because: {} ", e.what());
      }
    }
  }
}

void SimDataBroadcaster::Run(absl::Duration delay) {
  while (true) {
    sim_data_ = ConvertSimData();
    server_->Broadcast(sim_data_.SerializeAsString());
    //server_->Broadcast("Hello world");
    absl::SleepFor(delay);
  }
}
SimData SimDataBroadcaster::ConvertSimData() { 
  // produce fake data for debugging.
  return FakeSimData();
}
}  // namespace ws
}  // namespace flight_panel
