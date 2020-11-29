#include "data_def/util.h"
#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

namespace flight_panel {
namespace data {
namespace {

TEST(DataUtilText, TestTransponderCodeConvert) {
  // Converts the decimal value to hex string.
  EXPECT_EQ("1234", DecodeTransponder(4660.0));
}


TEST(DataUtilText, TestToSimData) {
  // Converts the decimal value to hex string.
  SimVars input;
  spdlog::info("Test started");
  input.adiBank = 50;
  strncpy(input.atcCallSign, "HAPPY", 5);
  SimData output = ToSimData(input);
  EXPECT_EQ(output.instruments().bank_angle(), 50);
  EXPECT_EQ(output.aircraft_info().call_sign(), "HAPPY");
  spdlog::info("Test finished");
}
}  // namespace
}  // namespace data
}  // namespace flight_panel