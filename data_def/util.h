#pragma once

#include "data_def/proto/sim_data.pb.h"
#include "data_def/sim_vars.h"

namespace flight_panel {
namespace data {
std::string DecodeTransponder(double value);

// Converts SimVars struct to SimData proto.
SimData ToSimData(const SimVars& src);

}  // namespace data
}  // namespace flight_panel
