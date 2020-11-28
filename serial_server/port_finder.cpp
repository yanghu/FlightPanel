#include "port_finder.h"

#include <Wbemidl.h>

#include "absl/memory/memory.h"
#include "spdlog/spdlog.h"
#define _WIN32_DCOM
#include <Wbemidl.h>
#include <comdef.h>

#include <codecvt>
#include <iostream>

#pragma comment(lib, "wbemuuid.lib")

namespace flight_panel {
namespace serial {
namespace {
using namespace std;

class PortFinderImpl : public PortFinder {
 public:
  PortFinderImpl()
      : pSvc_(nullptr), pLoc_(nullptr), pEnumerator_(nullptr), isClean_(true){};
  ~PortFinderImpl();
  virtual std::vector<SerialDevice> GetComPort(const std::string& vid,
                                               const std::string& pid) override;
  virtual std::vector<SerialDevice> GetComPort() override;

 private:
  int Connect();
  int Clean();
  bool isClean_;
  IWbemServices* pSvc_;
  IWbemLocator* pLoc_;
  IEnumWbemClassObject* pEnumerator_;
};
PortFinderImpl::~PortFinderImpl() { Clean(); }

std::vector<SerialDevice> PortFinderImpl::GetComPort(const std::string& vid,
                                                     const std::string& pid) {
  std::vector<SerialDevice> devices;
  std::string vidStr = "VID_" + vid;
  std::string pidStr = "PID_" + pid;
  for (auto device : GetComPort()) {
    const std::string& id = device.pnpID;
    SPDLOG_DEBUG("Device PNP ID: {}", device.pnpID);

    if (id.find(pidStr) != std::string::npos &&
        id.find(vidStr) != std::string::npos) {
      devices.push_back(device);
    }
  }
  return devices;
};

std::vector<SerialDevice> PortFinderImpl::GetComPort() {
  Connect();
  auto result = std::vector<SerialDevice>();

  HRESULT hres;
  // Step 6: --------------------------------------------------
  // Use the IWbemServices pointer to make requests of WMI ----

  // For example, get the name of the operating system
  pEnumerator_ = NULL;
  hres =
      pSvc_->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_SerialPort"),
                       WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                       NULL, &pEnumerator_);

  if (FAILED(hres)) {
    cout << "Query for serial ports failed."
         << " Error code = 0x" << hex << hres << endl;
    return result;  // Program has failed.
  }

  // Step 7: -------------------------------------------------
  // Get the data from the query in step 6 -------------------

  IWbemClassObject* pclsObj = NULL;
  ULONG uReturn = 0;

  while (pEnumerator_) {
    HRESULT hr = pEnumerator_->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

    if (0 == uReturn) {
      break;
    }

    VARIANT vtProp;

    // Get the value of the Name property
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    hr = pclsObj->Get(L"Description", 0, &vtProp, 0, 0);
    std::wstring wName = vtProp.bstrVal;
    std::string name = converter.to_bytes(wName);

    VariantClear(&vtProp);
    hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
    std::wstring wComPort = vtProp.bstrVal;
    std::string comPort = converter.to_bytes(wComPort);
    VariantClear(&vtProp);
    hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
    std::wstring wPnpDeviceID = vtProp.bstrVal;
    std::string pnpDeviceID = converter.to_bytes(wPnpDeviceID);
    VariantClear(&vtProp);

    result.push_back({name, pnpDeviceID, comPort});
    pclsObj->Release();
  }

  Clean();
  return result;
};

int PortFinderImpl::Connect() {
  HRESULT hres;

  // Step 1: --------------------------------------------------
  // Initialize COM. ------------------------------------------

  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres)) {
    cout << "Failed to initialize COM library. Error code = 0x" << hex << hres
         << endl;
    return 1;  // Program has failed.
  }

  // Step 2: --------------------------------------------------
  // Set general COM security levels --------------------------

  hres = CoInitializeSecurity(
      NULL,
      -1,                           // COM authentication
      NULL,                         // Authentication services
      NULL,                         // Reserved
      RPC_C_AUTHN_LEVEL_DEFAULT,    // Default authentication
      RPC_C_IMP_LEVEL_IMPERSONATE,  // Default Impersonation
      NULL,                         // Authentication info
      EOAC_NONE,                    // Additional capabilities
      NULL                          // Reserved
  );

  if (FAILED(hres)) {
    cout << "Failed to initialize security. Error code = 0x" << hex << hres
         << endl;
    CoUninitialize();
    return 1;  // Program has failed.
  }

  // Step 3: ---------------------------------------------------
  // Obtain the initial locator to WMI -------------------------

  pLoc_ = NULL;

  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID*)&pLoc_);

  if (FAILED(hres)) {
    cout << "Failed to create IWbemLocator object."
         << " Err code = 0x" << hex << hres << endl;
    CoUninitialize();
    return 1;  // Program has failed.
  }

  // Step 4: -----------------------------------------------------
  // Connect to WMI through the IWbemLocator::ConnectServer method

  pSvc_ = NULL;

  // Connect to the root\cimv2 namespace with
  // the current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = pLoc_->ConnectServer(
      _bstr_t(L"ROOT\\CIMV2"),  // Object path of WMI namespace
      NULL,                     // User name. NULL = current user
      NULL,                     // User password. NULL = current
      0,                        // Locale. NULL indicates current
      NULL,                     // Security flags.
      0,                        // Authority (for example, Kerberos)
      0,                        // Context object
      &pSvc_                    // pointer to IWbemServices proxy
  );

  if (FAILED(hres)) {
    cout << "Could not connect. Error code = 0x" << hex << hres << endl;
    pLoc_->Release();
    CoUninitialize();
    return 1;  // Program has failed.
  }

  // cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;

  // Step 5: --------------------------------------------------
  // Set security levels on the proxy -------------------------

  hres = CoSetProxyBlanket(pSvc_,              // Indicates the proxy to set
                           RPC_C_AUTHN_WINNT,  // RPC_C_AUTHN_xxx
                           RPC_C_AUTHZ_NONE,   // RPC_C_AUTHZ_xxx
                           NULL,               // Server principal name
                           RPC_C_AUTHN_LEVEL_CALL,  // RPC_C_AUTHN_LEVEL_xxx
                           RPC_C_IMP_LEVEL_IMPERSONATE,  // RPC_C_IMP_LEVEL_xxx
                           NULL,                         // client identity
                           EOAC_NONE                     // proxy capabilities
  );

  if (FAILED(hres)) {
    cout << "Could not set proxy blanket. Error code = 0x" << hex << hres
         << endl;
    pSvc_->Release();
    pLoc_->Release();
    CoUninitialize();
    return 1;  // Program has failed.
  }

  isClean_ = false;
  return 0;  // Program successfully completed.
}

int PortFinderImpl::Clean() {
  if (isClean_) return 0;
  pSvc_->Release();
  pLoc_->Release();
  pEnumerator_->Release();
  CoUninitialize();
  isClean_ = true;
  return 0;
}
}  // namespace

std::unique_ptr<PortFinder> CreatePortFinder() {
  return absl::make_unique<PortFinderImpl>();
}
}  // namespace serial
}  // namespace flight_panel
