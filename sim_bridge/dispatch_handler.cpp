#include "dispatch_handler.h"

#include "absl_helper/status_macros.h"

namespace flight_panel {
namespace sim_bridge {

absl::Status GroupedDispatchHandler::OnStart() {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnStart());
  }
  return absl::OkStatus();
}

absl::Status GroupedDispatchHandler::OnStop() {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnStop());
  }
  return absl::OkStatus();
}

absl::Status GroupedDispatchHandler::OnData(int req_id, void* pData) {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnData(req_id, pData));
  }
  return absl::OkStatus();
}
}  // namespace sim_bridge
}  // namespace flight_panel
