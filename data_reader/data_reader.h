#pragma once

#include "absl/status/status.h"
#include "data_def/sim_vars.h"
#include "data_dispatcher/data_dispatcher.h"
#include "sim_bridge/dispatch_handler.h"
#include "sim_bridge/sim_bridge.h"

namespace flight_panel {

class DataReader : public sim_bridge::DispatchHandler {
 public:
  virtual absl::Status RegisterDataDef() = 0;
  // Start running, and dispatch new data when they arrive.
};

std::unique_ptr<DataReader> CreateDataReader(
    int request_id, int data_def_id, sim_bridge::SimBridge* bridge,
    data_dispatcher::DataDispatcher* dispatcher);

}  // namespace flight_panel
