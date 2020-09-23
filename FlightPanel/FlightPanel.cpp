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
  auto results = selector.GetComPort("2341", "8037");
  std::cout << results[0].comPort << std::endl;
  const char* comPort = results[0].comPort.c_str();

  Server server(comPort, datalink::Read());
  auto thread = std::thread(&Server::Run, &server);
  datalink::Run();
  thread.join();
  return 0;
}
