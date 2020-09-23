#include "Server.h"
#include <thread>

namespace flight_panel {
void Log(const std::string& msg) { std::cout << msg; }

flight_panel::Server::Server(const std::string& port, const SimVars* const sim)
    : sim_(sim), serial_(("\\\\.\\" + port).c_str()) {
}

void flight_panel::Server::Run() {
  int bytesRead = 0;
  while (true) {
    UpdateData();
    std::cout << "send size: " << sizeof(InstrumentData) << std::endl;
    if (!serial_.writeSerialPort((char*)&instrumentData_, sizeof(InstrumentData)))
      Log("Failed to write to serial port!");
    Sleep(500);

    bytesRead = serial_.readSerialPort(rBuf_, kBufSize);
    if (bytesRead > 0) {
      ProcessRead(bytesRead);
    }

  }
  return;
}

void Server::UpdateData() {
  instrumentData_.trimPos = (char)round(sim_->tfElevatorTrimIndicator*100);
  instrumentData_.flapCnt = (char)sim_->tfFlapsCount;
  instrumentData_.flapPos = (char)sim_->tfFlapsIndex;
  instrumentData_.landingGearPos = (char)sim_->gearPosition;
}

void Server::ProcessRead(int bytesRead) {

    printf("Read: %s", rBuf_);

}


}  // namespace flight_panel