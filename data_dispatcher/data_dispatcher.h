#pragma once
#include <functional>

#include "absl/status/status.h"
#include "data_def/sim_vars.h"

namespace flight_panel {
namespace data_dispatcher {

using DispatchCallback = std::function<absl::Status(const std::string&)>;
class DataDispatcher {
 public:
  // Start running, and dispatch new data when they arrive.
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual bool IsRunning() = 0;

  virtual absl::Status AddRecepient(
      DispatchCallback notify_callback)=0;
  // Notify the dispatch with new data.
  virtual absl::Status Notify(flight_panel::SimVars new_data) = 0;
};


std::unique_ptr<DataDispatcher> CreateDispatcher();

}  // namespace data_dispatcher
}  // namespace flight_panel
