#include "sim_bridge/sim_bridge.h"

#include "SimConnect.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "data_def/sim_vars.h"
#include "sim_bridge/dispatch_handler.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace sim_bridge {
namespace {

// Dispatch callback function.
// The context is a DispatchHandler pointer, which is used to handle SIM_START,
// SIM_STOP and data events.
void MyDispatchProcRd(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
  DispatchHandler* handler = static_cast<DispatchHandler*>(pContext);
  switch (pData->dwID) {
    case SIMCONNECT_RECV_ID_EVENT: {
      SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;
      switch (evt->uEventID) {
        case data::SIM_START:
          handler->OnStart();
          break;
        case data::SIM_STOP:
          handler->OnStop();
          break;
        default:
          SPDLOG_ERROR("Unknown event id: {:d}", evt->uEventID);
          break;
      }
      break;  // Case EVENT
    }
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA: {
      SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData =
          (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
      handler->OnData(pObjData->dwRequestID, &pObjData->dwData);
      break;
    }
    case SIMCONNECT_RECV_ID_QUIT: {
      handler->OnStop();
      break;
    }
  }
}

class SimBridgeImpl : public SimBridge {
 public:
  SimBridgeImpl();
  // Inherited via SimBridge
  virtual absl::Status Connect() override;
  virtual absl::Status CallDispatch(DispatchHandler* handler) override;
  virtual absl::Status RequestData(int req_id, int def_id,
                                   RefreshPeriod period) override;
  virtual absl::StatusOr<int> AddDataDef(int def_id, absl::string_view name,
                                  absl::string_view unit_or_type) override;
  virtual absl::Status MapClientEvent(int event_id,
                                      absl::string_view name) override;
  virtual absl::Status TransmitClientEvent(int event_id, double value) override;

 private:
  // SimConnect related objects.
  HANDLE hSimConnect_ = NULL;
  // pContext is "this" pointer
  static void DispatchProcRd(SIMCONNECT_RECV* pData, DWORD cbData,
                             void* pContext) {
    SimBridgeImpl* bridge = static_cast<SimBridgeImpl*>(pContext);
    return;
  }
};

SimBridgeImpl::SimBridgeImpl() {}

absl::Status SimBridgeImpl::Connect() {
  HRESULT result = SimConnect_Open(&hSimConnect_, "Bridge", NULL, 0, 0, 0);
  if (result < 0) {
    return absl::UnavailableError("Failed to connect to Sim");
  } else {
    return absl::OkStatus();
  }
}

absl::Status SimBridgeImpl::CallDispatch(DispatchHandler* handler) {
  SPDLOG_INFO("Call dispatch started");
  if (SimConnect_CallDispatch(hSimConnect_, MyDispatchProcRd, handler) < 0) {
    return absl::InternalError("Failed to call dispatch callback.");
  } else {
    SPDLOG_INFO("Call dispatch done");
    return absl::OkStatus();
  }
}

absl::Status SimBridgeImpl::RequestData(int req_id, int def_id,
                                        RefreshPeriod period) {
  SIMCONNECT_PERIOD fs_period = SIMCONNECT_PERIOD_SIM_FRAME;
  switch (period) {
    case RefreshPeriod::NEVER:
      fs_period = SIMCONNECT_PERIOD_NEVER;
      break;
    case RefreshPeriod::VISUAL_FRAME:
      fs_period = SIMCONNECT_PERIOD_VISUAL_FRAME;
      break;
    case RefreshPeriod::SECOND:
      fs_period = SIMCONNECT_PERIOD_SECOND;
      break;
    case RefreshPeriod::SIM_FRAME:
      fs_period = SIMCONNECT_PERIOD_SIM_FRAME;
      break;
    case RefreshPeriod::ONCE:
      fs_period = SIMCONNECT_PERIOD_ONCE;
      break;
  }
  if (SimConnect_RequestDataOnSimObject(hSimConnect_, req_id, def_id,
                                        SIMCONNECT_OBJECT_ID_USER, fs_period, 0,
                                        0, 0, 0) < 0) {
    return absl::InternalError(absl::StrFormat(
        "Failed to request data. Request ID: %d, Def ID: %d, Period: %d",
        req_id, def_id, fs_period));
  } else {
    return absl::OkStatus();
  }
}

absl::StatusOr<int> SimBridgeImpl::AddDataDef(int def_id, absl::string_view name,
                                       absl::string_view unit_or_type) {
  HRESULT result;
  int data_length = sizeof(double);
  // First find the data type.
  if (!absl::StrContains(unit_or_type, "string")) {
    // Type is not string. simply use name as unit. And data type is float64.
    result = SimConnect_AddToDataDefinition(hSimConnect_, def_id,
                                            std::string(name).c_str(),
                                            std::string(unit_or_type).c_str());

  } else {
    std::vector<std::string> num = absl::StrSplit(unit_or_type, "string");
    std::string message =
        absl::StrCat("Invalid data def type: ", name, unit_or_type);
    if (num.empty()) {
      SPDLOG_ERROR("cannot get number.");
      return absl::InvalidArgumentError(
          absl::StrCat(message, "| number is empty"));
    }
    if (!absl::SimpleAtoi(num[1], &data_length)) {
      return absl::InvalidArgumentError(
          absl::StrCat(message, "|cannot convert ", num[1], " to number."));
    };
    // Now length is either 8, 64 or 256.
    absl::flat_hash_map<int, SIMCONNECT_DATATYPE> type_map = {
        {8, SIMCONNECT_DATATYPE_STRING8},
        {64, SIMCONNECT_DATATYPE_STRING64},
        {256, SIMCONNECT_DATATYPE_STRING256}};
    if (!type_map.contains(data_length)) {
      return absl::InvalidArgumentError(
          absl::StrCat(message, "| invalid length: ", data_length));
    }
    result = SimConnect_AddToDataDefinition(hSimConnect_, def_id,
                                            std::string(name).c_str(), NULL,
                                            type_map[data_length]);
  }
  if (result != 0) {
    std::string msg = absl::StrCat("Failed to add data def: ", name);
    SPDLOG_ERROR(msg);
    return absl::InternalError(msg);
  }
  return data_length;
}

absl::Status SimBridgeImpl::MapClientEvent(int event_id,
                                           absl::string_view name) {
  if (SimConnect_MapClientEventToSimEvent(hSimConnect_, event_id,
                                          std::string(name).c_str()) != 0) {
    SPDLOG_ERROR("Map event failed: {}", std::string(name));
    return absl::InternalError(absl::StrCat("Failed to map event: ", name));
  }
  return absl::OkStatus();
}

absl::Status SimBridgeImpl::TransmitClientEvent(int event_id, double value) {
  auto result =
      SimConnect_TransmitClientEvent(hSimConnect_, 0, event_id, (DWORD)value,
                                     SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                                     SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
  if (result != 0) {
    SPDLOG_ERROR("Failed to transmit client event: {}", event_id);
    return absl::InternalError("Failed to transmit client event.");
  } else {
    return absl::OkStatus();
  }
}

}  // namespace

std::unique_ptr<SimBridge> CreateSimBridge() {
  return absl::make_unique<SimBridgeImpl>();
}
}  // namespace sim_bridge

}  // namespace flight_panel
