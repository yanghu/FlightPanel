// FlightPanel.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

#include <Windows.h>

#include <iostream>
#include <string>

#include "DataLink.h"
#include "SerialPort.hpp"
#include "SerialSelect.h"
void ComServer(const std::wstring& port) {
  std::wstring modifiedPort = L"\\\\.\\" + port;
  char* comPort = (char*)malloc(50);
  wcstombs_s(comPort, 6, port.c_str(), 50);

  printf("%s", comPort);
  SerialPort arduino = SerialPort(comPort);
  char* buffer = (char*)malloc(10);
  buffer[0] = 't';
  arduino.writeSerialPort(buffer, 1);
  return;
};
int main() {
  using namespace flight_panel;
  SerialSelect selector{};
  auto results = selector.GetComPort(L"2341", L"8037");
  for (auto result : results) {
    std::wcout << "Name: " << result.name << "| Port: " << result.comPort
               << "| ID: " << result.pnpID << std::endl;
  }
  std::wcout << results[0].comPort << std::endl;

  auto server = std::thread(ComServer, results[0].comPort);
  datalink::Run();
  server.join();
  return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add
//   Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project
//   and select the .sln file
