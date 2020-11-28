// Selects serial ports based on vid and pid. Returns the serial device.
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace flight_panel {
namespace serial {

struct SerialDevice {
  std::string name;
  std::string pnpID;
  std::string comPort;
};

class PortFinder {
 public:
  virtual std::vector<SerialDevice> GetComPort(const std::string& vid,
                                               const std::string& pid) = 0;
  virtual std::vector<SerialDevice> GetComPort() = 0;
};

std::unique_ptr<PortFinder> CreatePortFinder();
}  // namespace serial
}  // namespace flight_panel
