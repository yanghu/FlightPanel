#include "data_reader/data_reader.h"

#include "absl/functional/bind_front.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "data_def/proto/sim_data.pb.h"
#include "data_dispatcher/data_dispatcher.h"
#include "data_dispatcher/mock_data_dispatcher.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "sim_bridge/mock_sim_bridge.h"
#include "sim_bridge/sim_bridge.h"

namespace flight_panel {

namespace {

using data::SimVars;
using data_dispatcher::DataDispatcher;
using data_dispatcher::MockDataDispatcher;
using sim_bridge::MockSimBridge;
using sim_bridge::SimBridge;
using ::testing::_;
using ::testing::DoubleEq;
using ::testing::Field;
using ::testing::Return;
using ::testing::SaveArg;

MATCHER_P(IsStringType, length, "Check if string type") {
  return arg == absl::StrCat("string", length);
}

MATCHER(IsNotStringType, "Check if string type") {
  return !absl::StrContains(arg, "string");
}

int DataDefCount() {
  int i = 0;
  while (data::SimVarDefs[i][0] != NULL) ++i;
  return i;
}

TEST(DataReaderTest, TestAddDataDefForAllEntries) {
  MockSimBridge mock_bridge;
  MockDataDispatcher mock_dispatcher;
  auto reader = CreateDataReader(0, 0, &mock_bridge, &mock_dispatcher);
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, _))
      .Times(DataDefCount())
      .WillRepeatedly(Return(8));
  reader->RegisterDataDef();
}

TEST(DataReaderTest, TestDataLengthCorrect) {
  MockSimBridge mock_bridge;
  MockDataDispatcher mock_dispatcher;
  auto reader = CreateDataReader(0, 0, &mock_bridge, &mock_dispatcher);
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsNotStringType()))
      .WillRepeatedly(Return(sizeof(double)));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(8)))
      .WillRepeatedly(Return(8));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(64)))
      .WillRepeatedly(Return(64));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(256)))
      .WillRepeatedly(Return(256));
  reader->RegisterDataDef();
  SimVars buffer;
  // we have another field for "connected" in SimVars, which is not part of the
  // SimConnect data frame.
  EXPECT_EQ(reader->DataLength() + 8, sizeof(buffer));
}

TEST(DataReaderTest, TestOnDataCopyOver) {
  MockSimBridge mock_bridge;
  MockDataDispatcher mock_dispatcher;
  auto reader = CreateDataReader(0, 0, &mock_bridge, &mock_dispatcher);
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsNotStringType()))
      .WillRepeatedly(Return(sizeof(double)));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(8)))
      .WillRepeatedly(Return(8));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(64)))
      .WillRepeatedly(Return(64));
  EXPECT_CALL(mock_bridge, AddDataDef(0, _, IsStringType(256)))
      .WillRepeatedly(Return(256));
  SimVars notified_data;
  EXPECT_CALL(mock_dispatcher, Notify(_))
      .WillOnce(DoAll(SaveArg<0>(&notified_data), Return(absl::OkStatus())));
  reader->RegisterDataDef();
  SimVars buffer;
  // Modify the buffer so it's different from the default value.
  buffer.asiAirspeed = 99;
  buffer.altAltitude = 555;
  strncpy(buffer.atcTailNumber, "HELLO", 5);
  // We will verify if the "connected" is copied over or not.
  buffer.connected = 1;
  reader->OnData(0, &buffer);
  // Verify that the data sent to the dispatcher matches the buffer
  EXPECT_EQ(notified_data.asiAirspeed, 99);
  EXPECT_EQ(notified_data.altAltitude, 555);
  EXPECT_EQ(std::string(notified_data.atcTailNumber), "HELLO");
  EXPECT_EQ(notified_data.connected, 0);
}

}  // namespace
}  // namespace flight_panel