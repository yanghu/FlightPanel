#pragma once
#include <string>
#include <vector>

namespace flight_panel {

struct Device {
	std::wstring name;
	std::wstring pnpID;
	std::wstring comPort;
};

class SerialSelect
{
public:
	static std::vector<Device> GetComPort(const std::string& pid, const std::string& vid);
};

}