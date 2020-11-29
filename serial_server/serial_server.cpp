#include "serial_server/serial_server.h"

#include <string>
#include <thread>

#include "absl/memory/memory.h"
#include "absl/time/clock.h"
#include "serial_server/serial_port.h"
// #define TEST_LED

namespace flight_panel {
namespace serial {
namespace {

class SerialServerImpl : public SerialServer {
 public:
  SerialServerImpl(std::unique_ptr<SerialPort> port,
                   absl::Duration sendInterval = absl::Seconds(0.2));
  virtual void Run() override;
  virtual absl::Status SendData(const SimData& data) override;

  const static int kBufSize = 100;

 private:
  std::thread worker_;
  char rBuf_[kBufSize] = {0};
  InstrumentData instrumentData_;
  int bufSize_ = 0;
  std::unique_ptr<SerialPort> serial_;
  const absl::Duration sendInterval_;
  void UpdateData();
  void ProcessRead(int bytesRead);
};

void Log(const std::string& msg) { std::cout << msg; }

SerialServerImpl::SerialServerImpl(std::unique_ptr<SerialPort> port,
                                   absl::Duration sendInterval)
    : sendInterval_(sendInterval),
      serial_(std::move(port)),
      instrumentData_{0, 0, 0, 0} {}

void SerialServerImpl::Run() {
  int bytesRead = 0;
  while (true) {
    if (!serial_->isConnected()) {
      // Sleep 10 seconds.
      absl::SleepFor(absl::Seconds(10));
      continue;
    }
#ifdef TEST_LED
    instrumentData_ = InstrumentData{0, 3, 1, 80, 1};
#else
    UpdateData();
#endif
    if (!serial_->writeSerialPort((char*)&instrumentData_,
                                  sizeof(InstrumentData)))
      Log("Failed to write to serial port!");
    absl::SleepFor(sendInterval_);
    bytesRead = serial_->readSerialPort(rBuf_, kBufSize);
    if (bytesRead > 0) {
      ProcessRead(bytesRead);
    }
  }
  return;
}

// TODO(huyang): implement this.
absl::Status SerialServerImpl::SendData(const SimData& data) {
  return absl::UnimplementedError("Send data not implemented yet");
}

void SerialServerImpl::UpdateData(const SimData& data) {
  instrumentData_.trimPos = (char)round(sim_->tfElevatorTrimIndicator * 100);
  instrumentData_.flapCnt = (char)sim_->tfFlapsCount;
  instrumentData_.flapPos = (char)sim_->tfFlapsIndex;
  // Gear position is 0~1 float value. 1 is full extended.
  // Convert to 0~100 and send it as 8bit char.
  instrumentData_.landingGearPos = (char)(sim_->gearPosition * 100);
  instrumentData_.parkingBrakeOn = sim_->parkingBrakeOn;
}

void SerialServerImpl::ProcessRead(int bytesRead) { printf("Read: %s", rBuf_); }

}  // namespace

std::unique_ptr<SerialServer> CreateSerialServer(
    std::unique_ptr<SerialPort> serial_port, absl::Duration sendInterval) {
  return absl::make_unique<SerialServerImpl>(serial_port, sendInterval);
}
}  // namespace serial
}  // namespace flight_panel
