// Selects serial ports based on vid and pid. Returns the serial device.
#pragma once
#include <Wbemidl.h>

#include <string>
#include <vector>

namespace flight_panel {

struct SerialDevice {
  std::string name;
  std::string pnpID;
  std::string comPort;
};

class SerialSelect {
 public:
  SerialSelect() : pSvc_(nullptr), pLoc_(nullptr), pEnumerator_(nullptr), isClean_(true){};
  ~SerialSelect();
  std::vector<SerialDevice> GetComPort(const std::string& vid,
                                 const std::string& pid);
  std::vector<SerialDevice> GetComPort();
  int Connect();
  int Clean();

 private:
  bool isClean_;
  IWbemServices* pSvc_;
  IWbemLocator* pLoc_;
  IEnumWbemClassObject* pEnumerator_;
};

}  // namespace flight_panel