#pragma once
#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "sim_bridge/sim_bridge.h"

namespace flight_panel {
namespace sim_bridge {
class MockSimBridge : public SimBridge {
 public:
  MOCK_METHOD(absl::Status, Connect, (), (override));
  MOCK_METHOD(absl::Status, CallDispatch, (DispatchHandler * handler),
              (override));
  MOCK_METHOD(absl::Status, RequestData,
              (int req_id, int def_id, RefreshPeriod period), (override));
  MOCK_METHOD(absl::StatusOr<int>, AddDataDef,
              (int def_id, absl::string_view name,
               absl::string_view unit_or_type),
              (override));
  MOCK_METHOD(absl::Status, MapClientEvent,
              (int event_id, absl::string_view name), (override));

  MOCK_METHOD(absl::Status, SubscribeSystemEvent, (int, absl::string_view));
  MOCK_METHOD(absl::Status, TransmitClientEvent, (int event_id, double value),
              (override));
  MOCK_METHOD(absl::Status, Close, (), (override));
};
}  // namespace sim_bridge
}  // namespace flight_panel
