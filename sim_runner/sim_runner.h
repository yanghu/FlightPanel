#pragma once

#include <memory>

#include "absl/status/status.h"
#include "data_dispatcher/data_dispatcher.h"
#include "sim_bridge/sim_bridge.h"

namespace flight_panel {
class SimRunner {
 public:
  virtual absl::Status Init() = 0;
  virtual absl::Status Run() = 0;
  virtual absl::Status Stop() = 0;

  virtual sim_bridge::SimBridge* sim_bridge() = 0;
  virtual data_dispatcher::DataDispatcher* data_dispatcher() = 0;
};

std::unique_ptr<SimRunner> CreateSimRunner(
    std::unique_ptr<sim_bridge::SimBridge> sim_bridge,
    std::unique_ptr<data_dispatcher::DataDispatcher> data_dispatcher);

}  // namespace flight_panel
