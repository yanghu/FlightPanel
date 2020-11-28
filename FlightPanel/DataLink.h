#pragma once
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <string>

#include <thread>

#include "data_def/sim_vars.h"
#include "SimConnect.h"

namespace flight_panel {
namespace datalink {
int Run(const std::string& inputSerialPort);
const SimVars* const Read();
}  // namespace datalink
}  // namespace flight_panel
