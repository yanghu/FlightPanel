// FlightPanel.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

#include <Windows.h>

#include <iostream>
#include <string>

#include "DataLink.h"
#include "SerialPort.hpp"
#include "SerialSelect.h"
#include "Server.h"

int main() {
  using namespace flight_panel;
  SerialSelect selector{};
  auto results = selector.GetComPort("2340", "8030");
  const char* inputComPort;
  if (results.size() == 0) {
    inputComPort = "";
  } else {
    inputComPort = results[0].comPort.c_str();
    std::cout << results[0].comPort << std::endl;
  }
    
  const char myPort[] = "COM21";
 
  // Serial port server to communicate with the panel motors.
  Server server(myPort, datalink::Read(), 500);
  auto thread = std::thread(&Server::Run, &server);
  // Runs the datalink
  datalink::Run(inputComPort);
  thread.join();
  return 0;
}
