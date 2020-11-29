#include "data_dispatcher/data_dispatcher.h"

#include "absl/functional/bind_front.h"
#include "absl/status/status.h"
#include "absl/time/clock.h"
#include "data_def/proto/sim_data.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace flight_panel {
namespace data_dispatcher {

namespace {
using ::testing::_;

class MockClient {
 public:
  MOCK_METHOD(absl::Status, Dispatch, (const SimData&));
};

TEST(DataDispatcherTest, TestNotifyTriggersDispatch) {
  MockClient mock_client;
  SimVars data;
  EXPECT_CALL(mock_client, Dispatch(_)).Times(2);
  std::unique_ptr<DataDispatcher> dispatcher = CreateDispatcher();
  dispatcher->AddRecepient(
      absl::bind_front(&MockClient::Dispatch, &mock_client));
  dispatcher->Start();
  dispatcher->Notify(data);
  dispatcher->Notify(data);
  absl::SleepFor(absl::Seconds(1));
  dispatcher->Stop();
}

TEST(DataDispatcherTest, TestIsRunningChecker) {
  MockClient mock_client;
  SimVars data;
  std::unique_ptr<DataDispatcher> dispatcher = CreateDispatcher();
  dispatcher->AddRecepient(
      absl::bind_front(&MockClient::Dispatch, &mock_client));
  EXPECT_FALSE(dispatcher->IsRunning());
  dispatcher->Start();
  dispatcher->Notify(data);
  EXPECT_TRUE(dispatcher->IsRunning());
  dispatcher->Stop();
  EXPECT_FALSE(dispatcher->IsRunning());
}
}  // namespace
}  // namespace data_dispatcher
}  // namespace flight_panel