// FlightPanel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <Windows.h>
#include "SerialSelect.h"

bool SelectComPort() //added function to find the present serial 
{
    wchar_t  lpTargetPath[5000]; // buffer to store the path of the COMPORTS
    bool gotPort = false; // in case the port is not found

    for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
    {
        std::wstring portName = L"COM" + std::to_wstring(i); // converting to COM0, COM1, COM2
        DWORD test = QueryDosDevice(portName.c_str(), lpTargetPath, 5000);

        // Test the return value and error if any
        if (test != 0) //QueryDosDevice returns zero if it didn't find an object
        {
            std::wcout << portName << ": " << lpTargetPath << std::endl;
            gotPort = true;
        }

        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
        }
    }

    return gotPort;
}

int main() {
    SelectComPort();
    SerialSelect::GetComPort("hell", "bbb");
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
