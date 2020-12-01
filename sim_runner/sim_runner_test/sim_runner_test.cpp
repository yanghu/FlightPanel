#include "sim_runner/sim_runner.h"

#include "absl/status/status.h"
#include "absl/time/clock.h"
#include "data_def/proto/sim_data.pb.h"
#include "data_dispatcher/mock_data_dispatcher.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "sim_bridge/mock_sim_bridge.h"

namespace flight_panel {

namespace {
using ::testing::_;
using ::testing::DoubleEq;
using ::testing::Field;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

using data_dispatcher::MockDataDispatcher;
using sim_bridge::MockSimBridge;

using data::SimVars;

class MockClient {
 public:
  MOCK_METHOD(absl::Status, Dispatch, (const SimData&));
};

TEST(SimRunnerTest, TestCreateSimRunner) {
  auto mock_bridge = absl::make_unique<MockSimBridge>();
  auto mock_dispatcher = absl::make_unique<MockDataDispatcher>();
  auto runner =
      CreateSimRunner(std::move(mock_bridge), std::move(mock_dispatcher));
}

TEST(SimRunnerTest, TestInit_SuccessConnect_WillAddDataRef) {
  auto mock_bridge = new MockSimBridge();
  auto mock_dispatcher = new MockDataDispatcher();
  auto runner =
      CreateSimRunner(absl::WrapUnique<MockSimBridge>(mock_bridge),
                      absl::WrapUnique<MockDataDispatcher>(mock_dispatcher));

  // Setup mocks
  EXPECT_CALL(*mock_bridge, Connect()).WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(*mock_bridge, AddDataDef(0, _, _)).WillRepeatedly(Return(8));
  EXPECT_CALL(*mock_bridge, SubscribeSystemEvent(0, _))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(*mock_bridge, SubscribeSystemEvent(1, _))
      .WillOnce(Return(absl::OkStatus()));

  runner->Init();
}


TEST(SimRunnerTest, TestRunWithoutInit_Fail) {
  auto mock_bridge = new MockSimBridge();
  auto mock_dispatcher = new MockDataDispatcher();
  auto runner =
      CreateSimRunner(absl::WrapUnique<MockSimBridge>(mock_bridge),
                      absl::WrapUnique<MockDataDispatcher>(mock_dispatcher));
  // Setup mocks
  EXPECT_EQ(runner->Run().code(), absl::StatusCode::kFailedPrecondition);
}

}  // namespace
}  // namespace flight_panel