#include "dispatch_handler.h"

#include "absl_helper/status_macros.h"

inline absl::Status
flight_panel::sim_bridge::GroupedDispatchHandler::OnStart() {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnStart());
  }
  return absl::OkStatus();
}

inline absl::Status flight_panel::sim_bridge::GroupedDispatchHandler::OnStop() {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnStop());
  }
  return absl::OkStatus();
}

inline absl::Status flight_panel::sim_bridge::GroupedDispatchHandler::OnData(
    int req_id, void* pData) {
  for (auto handler : handlers_) {
    RETURN_IF_ERROR(handler->OnData(req_id, pData));
  }
  return absl::OkStatus();
}
