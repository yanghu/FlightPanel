#pragma once

#include "data_def/sim_vars.h"
#include "absl/status/status.h"
#include "data_dispatcher/data_dispatcher.h"
#include "gmock/gmock.h"

namespace flight_panel {
namespace data_dispatcher {

class MockDataDispatcher : public DataDispatcher {
 public:
  MOCK_METHOD(void, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(bool, IsRunning, (), (override));
  MOCK_METHOD(int, QueueSize, (), (override));
  MOCK_METHOD(absl::Status, AddRecepient, (DispatchCallback notify_callback), (override));
  MOCK_METHOD(absl::Status, Notify, (flight_panel::data::SimVars new_data), (override));
};

}  // namespace data_dispatcher
}  // namespace flight_panel
