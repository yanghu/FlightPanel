#pragma once
#include <iostream>
#include <string>

#include "DataLink.h"
#include "SerialPort.hpp"
#include "SimVars.h"

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
};

class Server {
 public:
  Server(const std::string& port, const SimVars* const sim);
  void Run();
  const static int kBufSize = 100;

 private:
  std::thread worker_;
  char wBuf_[kBufSize];
  char rBuf_[kBufSize];
  InstrumentData instrumentData_;
  int bufSize_ = 0;
  const SimVars* const sim_;
  SerialPort serial_;
  void UpdateData();
  void ProcessRead(int bytesRead);
};

}  // namespace flight_panel
