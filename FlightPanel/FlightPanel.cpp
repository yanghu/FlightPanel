// FlightPanel.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <memory>

#include "serial_server/port_finder.h"
#include "serial_server/serial_server.h"
#include "DataLink.h"

#include "websocket_server/websocket_server.h"
#include "spdlog/spdlog.h"
#include <Windows.h>

#define LOG_INFO SPDLOG_INFO

// See: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
// Example output:
//   [23:31:01]|[DataLink.cpp:186][info]: Input com port:
void SetupLogger() { spdlog::set_pattern("[%T]|[%s:%#]%^[%l]%$: %v"); }

int main() {
  using namespace flight_panel;
  SetupLogger();
  auto port_finder = flight_panel::serial::CreatePortFinder();
  // Get the com port of the ProMicro board. The PID/VID are customized.
  auto results = port_finder->GetComPort("2340", "8030");
  const char* inputComPort;
  if (results.size() == 0) {
    inputComPort = "";
  } else {
    inputComPort = results[0].comPort.c_str();
    LOG_INFO("Input Serial COM port: {}", results[0].comPort);
  }

  // Port number of my UNO. the UNO's port cannot be obtained using "GetComPort"
  // so we have to hard code it.
  const char myPort[] = "COM21";

  // Serial port server to communicate with the panel motors and LEDs.
  auto server = serial::CreateSerialServer(
      serial::CreateSerialPort(myPort), absl::Milliseconds(500)
  );
  auto serial_thread = std::thread(&serial::SerialServer::Run, server.get());

  ws::WebSocketServer wsServer;
  auto ws_thread = std::thread(&ws::WebSocketServer::Run, &wsServer, 8080);
  auto ws_event_thread =
      std::thread(&ws::WebSocketServer::ProcessEvents, &wsServer);

  ws::SimDataBroadcaster broadcaster(
      std::unique_ptr<ws::WebSocketServer>(&wsServer), datalink::Read());
  auto broadcast_thread =
      std::thread(&ws::SimDataBroadcaster::Run, &broadcaster, 
        absl::Milliseconds(10));

  // Runs the datalink
  datalink::Run(inputComPort);

  serial_thread.join();
  ws_thread.join();
  ws_event_thread.join();
  broadcast_thread.join();
  return 0;
}
