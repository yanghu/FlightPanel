#pragma once
#include <Wbemidl.h>

#include <string>
#include <vector>

namespace flight_panel {

struct Device {
  std::wstring name;
  std::wstring pnpID;
  std::wstring comPort;
};

class SerialSelect {
 public:
  SerialSelect() : pSvc_(nullptr), pLoc_(nullptr), pEnumerator_(nullptr), isClean_(true){};
  ~SerialSelect();
  std::vector<Device> GetComPort(const std::wstring& vid,
                                 const std::wstring& pid);
  std::vector<Device> GetComPort();
  int Connect();
  int Clean();

 private:
  bool isClean_;
  IWbemServices* pSvc_;
  IWbemLocator* pLoc_;
  IEnumWbemClassObject* pEnumerator_;
};

}  // namespace flight_panel