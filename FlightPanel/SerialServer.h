#pragma once
#include <iostream>
#include <string>

#include "DataLink.h"
#include "SerialPort.hpp"
#include "data_def/sim_vars.h"

namespace flight_panel {

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
  SerialServer(const std::string& port, const SimVars* const sim, int sendIntervalMilliseconds = 200);
  void Run();
  const static int kBufSize = 100;

 private:
  std::thread worker_;
  char rBuf_[kBufSize] = {0};
  InstrumentData instrumentData_;
  int bufSize_ = 0;
  const SimVars* const sim_;
  SerialPort serial_;
  const int sendIntervalMilliseconds_;
  void UpdateData();
  void ProcessRead(int bytesRead);
};

}  // namespace flight_panel
