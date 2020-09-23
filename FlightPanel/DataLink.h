#pragma once
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <thread>

#include "SimConnect.h"
#include "SimVars.h"

namespace flight_panel {
namespace datalink {
int Run();
const SimVars* const Read();

}  // namespace datalink
}  // namespace flight_panel
