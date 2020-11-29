#include "serial_server/serial_port.h"

#include <windows.h>

#include <iostream>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
/*
 * Author: Manash Kumar Mandal
 * Modified by: Yang Hu
 * Modified Library introduced in Arduino Playground which does not work
 * This works perfectly
 * LICENSE: MIT
 */

namespace flight_panel {
namespace serial {
namespace {

constexpr int kArduinoWaitTime = 2000;

class SerialPortImpl : public SerialPort {
 public:
  explicit SerialPortImpl(const char *portName);
  ~SerialPortImpl();

  virtual int readSerialPort(const char *buffer,
                             unsigned int buf_size) override;
  virtual bool writeSerialPort(const char *buffer,
                               unsigned int buf_size) override;
  virtual bool isConnected() override;
  virtual void closeSerial() override;

 private:
  HANDLE handler;
  bool connected;
  COMSTAT status;
  DWORD errors;
};

SerialPortImpl::SerialPortImpl(const char *portName) {
  this->connected = false;

  this->handler =
      CreateFileA(static_cast<LPCSTR>(portName), GENERIC_READ | GENERIC_WRITE,
                  0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (this->handler == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      std::cerr << "ERROR: Handle was not attached.Reason : " << portName
                << " not available\n";
    } else {
      std::cerr << "ERROR!!!\n";
    }
  } else {
    DCB dcbSerialParameters = {0};

    if (!GetCommState(this->handler, &dcbSerialParameters)) {
      std::cerr << "Failed to get current serial parameters\n";
    } else {
      dcbSerialParameters.BaudRate = CBR_9600;
      dcbSerialParameters.ByteSize = 8;
      dcbSerialParameters.StopBits = ONESTOPBIT;
      dcbSerialParameters.Parity = NOPARITY;
      dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

      if (!SetCommState(handler, &dcbSerialParameters)) {
        std::cout << "ALERT: could not set serial port parameters\n";
      } else {
        this->connected = true;
        PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
        Sleep(kArduinoWaitTime);
      }
    }
  }
}

SerialPortImpl::~SerialPortImpl() {
  if (this->connected) {
    this->connected = false;
    CloseHandle(this->handler);
  }
}

// Reading bytes from serial port to buffer;
// returns read bytes count, or if error occurs, returns 0
int SerialPortImpl::readSerialPort(const char *buffer, unsigned int buf_size) {
  DWORD bytesRead{};
  unsigned int toRead = 0;

  ClearCommError(this->handler, &this->errors, &this->status);

  if (this->status.cbInQue > 0) {
    if (this->status.cbInQue > buf_size) {
      toRead = buf_size;
    } else {
      toRead = this->status.cbInQue;
    }
  }

  memset((void *)buffer, 0, buf_size);

  if (ReadFile(this->handler, (void *)buffer, toRead, &bytesRead, NULL)) {
    return bytesRead;
  }

  return 0;
}

// Sending provided buffer to serial port;
// returns true if succeed, false if not
bool SerialPortImpl::writeSerialPort(const char *buffer,
                                     unsigned int buf_size) {
  DWORD bytesSend;

  if (!WriteFile(this->handler, (void *)buffer, buf_size, &bytesSend, 0)) {
    ClearCommError(this->handler, &this->errors, &this->status);
    return false;
  }

  return true;
}

// Checking if serial port is connected
bool SerialPortImpl::isConnected() {
  if (!ClearCommError(this->handler, &this->errors, &this->status)) {
    this->connected = false;
  }

  return this->connected;
}

void SerialPortImpl::closeSerial() { CloseHandle(this->handler); }

}  // namespace

std::unique_ptr<SerialPort> CreateSerialPort(absl::string_view port_name) {
  // Construct port name with prefix.
  std::string renamed_port = absl::StrCat("\\\\.\\", port_name);
  return absl::make_unique<SerialPortImpl>(renamed_port.c_str());
}

}  // namespace serial
}  // namespace flight_panel
