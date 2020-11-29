// This class is a wrapper of the SimConnect API.
#pragma once

#include <windows.h>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "data_def/sim_vars.h"
#include "sim_bridge/dispatch_handler.h"

namespace flight_panel {
namespace sim_bridge {

// Refresh period used when request data.
enum class RefreshPeriod { NEVER, ONCE, VISUAL_FRAME, SIM_FRAME, SECOND };
class SimBridge;
class DispatchHandler;

// SimBridge is a wrapper around the SimConnect API.
// It provides a C++ style interface to the Sim.
class SimBridge {
 public:
  virtual absl::Status Connect() = 0;
  virtual absl::Status CallDispatch(DispatchHandler* handler) = 0;
  virtual absl::Status RequestData(int req_id, int def_id,
                                   RefreshPeriod period) = 0;

  // The unit or type field can accept "string{8|64}", or a unit string.
  // When the type is not string, default(float64) type is used.
  // Returns data length of the added def.
  virtual absl::StatusOr<int> AddDataDef(int def_id, absl::string_view name,
                                  absl::string_view unit_or_type) = 0;

  virtual absl::Status MapClientEvent(int event_id, absl::string_view name) = 0;
  virtual absl::Status TransmitClientEvent(int event_id, double value) = 0;
};
std::unique_ptr<SimBridge> CreateSimBridge();

}  // namespace sim_bridge
}  // namespace flight_panel
