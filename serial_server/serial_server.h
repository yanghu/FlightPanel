
#pragma once
#include <iostream>
#include <string>

#include "absl/time/clock.h"
#include "absl/status/status.h"
#include "data_def/sim_vars.h"
#include "data_def/proto/sim_data.pb.h"
#include "serial_server/serial_port.h"

namespace flight_panel {
namespace serial {

struct InstrumentData {
  // -100 to 100, -1 means nose down.
  char trimPos;
  // Total number of flap positions
  char flapCnt;
  // Flap position index. [0, flapCnt]
  char flapPos;
  // 0-unkonwn, 1-up, 2-down.
  char landingGearPos;
  bool parkingBrakeOn;
};

class SerialServer {
 public:
  virtual void Run() = 0;
  virtual absl::Status SendData(const SimData& data)=0;
};

std::unique_ptr<SerialServer> CreateSerialServer(
    std::unique_ptr<SerialPort> serial_port, absl::Duration sendInterval);
}  // namespace serial
}  // namespace flight_panel
