#pragma once
#include <vector>

#include "absl/status/status.h"

namespace flight_panel {
namespace sim_bridge {

// The data handler interface, uesd in dispatch callback.
class DispatchHandler {
 public:
  virtual absl::Status OnStart() = 0;
  virtual absl::Status OnStop() = 0;
  // The caller who setup the data request should know the layout of data type
  // and is responsible for converting the data pointer to correct type and
  // size.
  virtual absl::Status OnData(int req_id, void* pData) = 0;
};

// A handler that groups other handlers.
class GroupedDispatchHandler : DispatchHandler {
 public:
  GroupedDispatchHandler(){};
  GroupedDispatchHandler(const std::vector<DispatchHandler*> handlers)
      : handlers_(handlers){};

  void AddHandler(DispatchHandler* handler) { handlers_.push_back(handler); }

  absl::Status OnStart();
  absl::Status OnStop();
  absl::Status OnData(int req_id, void* pData);

 private:
  std::vector<DispatchHandler*> handlers_;
};

}  // namespace sim_bridge
}  // namespace flight_panel
