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
using ::testing::DoubleEq;
using ::testing::Field;
using ::testing::Return;
using ::testing::SaveArg;
using data::SimVars;

class MockClient {
 public:
  MOCK_METHOD(absl::Status, Dispatch, (const SimData&));
};

TEST(DataDispatcherTest, TestNotifyTriggersDispatch) {
  MockClient mock_client1, mock_client2;
  SimVars data;
  EXPECT_CALL(mock_client1, Dispatch(_)).Times(2);
  EXPECT_CALL(mock_client2, Dispatch(_)).Times(2);
  std::unique_ptr<DataDispatcher> dispatcher = CreateDispatcher();
  dispatcher->AddRecepient(
      absl::bind_front(&MockClient::Dispatch, &mock_client1));
  dispatcher->AddRecepient(
      absl::bind_front(&MockClient::Dispatch, &mock_client2));
  dispatcher->Start();
  dispatcher->Notify(data);
  dispatcher->Notify(data);
  while (dispatcher->QueueSize() > 0) {
    ;
  }
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

TEST(DataDispatcherTest, TestDataConverted) {
  MockClient mock_client;
  SimVars data;
  data.adiBank = 50;
  SimData dispatched;
  EXPECT_CALL(mock_client, Dispatch(_))
      .WillOnce(
          ::testing::DoAll(SaveArg<0>(&dispatched), Return(absl::OkStatus())));
  std::unique_ptr<DataDispatcher> dispatcher = CreateDispatcher();
  dispatcher->AddRecepient(
      absl::bind_front(&MockClient::Dispatch, &mock_client));
  dispatcher->Start();
  dispatcher->Notify(data);
  while (dispatcher->QueueSize() > 0) {
    ;
  }
  dispatcher->Stop();
  EXPECT_THAT(dispatched.instruments().bank_angle(), DoubleEq(50));
}

}  // namespace
}  // namespace data_dispatcher
}  // namespace flight_panel