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
  //SerialSelect selector{};
  // auto results = selector.GetComPort("1A86", "7523");
  //if (results.size() == 0) return -1;
  //std::cout << results[0].comPort << std::endl;
  //const char* comPort = results[0].comPort.c_str();
  const char myPort[] = "COM19";
 
  // Serial port server to communicate with the panel motors.
  Server server(myPort, datalink::Read(), 500);
  auto thread = std::thread(&Server::Run, &server);
  // Runs the datalink
  datalink::Run();
  thread.join();
  return 0;
}
