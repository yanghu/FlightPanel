#include "sim_bridge/sim_bridge.h"

#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace sim_bridge {
namespace {

TEST(TestSimBridge, TestAddValidDataDef) {
  // For valid data def types, should only fail with internal error, not invalid
  // argument error.
  auto bridge = CreateSimBridge();
  EXPECT_EQ(bridge->AddDataDef(0, "hello", "string8").status().code(),
            absl::StatusCode::kInternal);
  EXPECT_EQ(bridge->AddDataDef(0, "hello", "string256").status().code(),
            absl::StatusCode::kInternal);
  EXPECT_EQ(bridge->AddDataDef(0, "hello", "degree").status().code(),
            absl::StatusCode::kInternal);
}

TEST(TestSimBridge, TestAddInvalidDataRef) {
  auto bridge = CreateSimBridge();
  EXPECT_EQ(bridge->AddDataDef(0, "hello", "string").status().code(),
            absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(bridge->AddDataDef(0, "hello", "string38").status().code(),
            absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace sim_bridge
}  // namespace flight_panel
