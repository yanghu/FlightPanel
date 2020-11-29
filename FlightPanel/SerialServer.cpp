#include "SerialServer.h"

#include <thread>
// #define TEST_LED

namespace flight_panel {
void Log(const std::string& msg) { std::cout << msg; }

flight_panel::SerialServer::SerialServer(const std::string& port,
                                         const data::SimVars* const sim,
                                         int sendIntervalMilliseconds)
    : sim_(sim),
      sendIntervalMilliseconds_(sendIntervalMilliseconds),
      serial_(("\\\\.\\" + port).c_str()),
      instrumentData_{0, 0, 0, 0} {}

void flight_panel::SerialServer::Run() {
  int bytesRead = 0;
  while (true) {
    if (!serial_.isConnected()) {
      // Sleep 10 seconds.
      Sleep(10000);
      continue;
    }
#ifdef TEST_LED
    instrumentData_ = InstrumentData{0, 3, 1, 80, 1};
#else
    UpdateData();
#endif
    if (!serial_.writeSerialPort((char*)&instrumentData_,
                                 sizeof(InstrumentData)))
      Log("Failed to write to serial port!");
    Sleep(sendIntervalMilliseconds_);
    bytesRead = serial_.readSerialPort(rBuf_, kBufSize);
    if (bytesRead > 0) {
      ProcessRead(bytesRead);
    }
  }
  return;
}

void SerialServer::UpdateData() {
  instrumentData_.trimPos = (char)round(sim_->tfElevatorTrimIndicator * 100);
  instrumentData_.flapCnt = (char)sim_->tfFlapsCount;
  instrumentData_.flapPos = (char)sim_->tfFlapsIndex;
  // Gear position is 0~1 float value. 1 is full extended.
  // Convert to 0~100 and send it as 8bit char.
  instrumentData_.landingGearPos = (char)(sim_->gearPosition * 100);
  instrumentData_.parkingBrakeOn = sim_->parkingBrakeOn;
}

void SerialServer::ProcessRead(int bytesRead) { printf("Read: %s", rBuf_); }

}  // namespace flight_panel