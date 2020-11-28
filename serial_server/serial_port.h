/*
 * Author: Manash Kumar Mandal
 * Modified by: Yang Hu
 *   Added abstract interface so this can be mocked in unit tests.
 * Modified Library introduced in Arduino Playground which does not work
 * This works perfectly
 * LICENSE: MIT
 */

#pragma once

#include <memory>

#include "absl/strings/string_view.h"

namespace flight_panel {
namespace serial {
class SerialPort {
 public:
  virtual int readSerialPort(const char *buffer, unsigned int buf_size) = 0;
  virtual bool writeSerialPort(const char *buffer, unsigned int buf_size) = 0;
  virtual bool isConnected() = 0;
  virtual void closeSerial() = 0;
};

std::unique_ptr<SerialPort> CreateSerialPort(absl::string_view port_name);
}  // namespace serial
}  // namespace flight_panel
