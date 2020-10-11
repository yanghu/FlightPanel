#include "DataLink.h"

#include <iostream>
#include <optional>

#include "SerialPort.hpp"
#include "SimConnect.h"
#include "SimVars.h"

namespace flight_panel {

extern const char* versionString;
extern const char* SimVarDefs[][2];
extern WriteEvent WriteEvents[];
namespace datalink {

enum DEFINITION_ID {
  // Definition that reads all variables defined in SimVar.h
  DEF_READ_ALL,
  DEF_WRITE,
};

enum REQUEST_ID {
  // The only request we would do is to read data.
  REQ_ID
};


enum GROUP_ID {
  GROUP0,
};

HANDLE hSimConnect = NULL;
bool quit = false;
SimVars simVars;
int varSize = 0;
// The first position is "connected", which is not part of the data read from
// MSFS. +1 to skip it.
double* varStart = (double*)&simVars + 1;
// Step
const double kTrimStep = 0.01;

// DispathProcRD is the callback to consume SimConnect data.using This is the
// callback that
void MyDispatchProcRd(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
  static int displayDelay = 0;
  static double newTrim = 0;
  // event type.
  switch (pData->dwID) {
    case SIMCONNECT_RECV_ID_EVENT: {
      SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;
      switch (evt->uEventID) {
        case SIM_START:
          break;
        case SIM_STOP:
          break;
        default:
          printf("Unknown event id: %ld\n", evt->uEventID);
          break;
      }
      break;  // Case EVENT
    }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA: {
      SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData =
          (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
      switch (pObjData->dwRequestID) {
        case REQ_ID:
          // Copy data to simVars_
          memcpy(varStart, &pObjData->dwData, varSize);
#ifdef DEBUG_VARS
          if (displayDelay > 0)
            displayDelay--;

          else {
            printf("Aircraft: %s   Cruise Speed: %f\n", simVars.aircraft,
                   simVars.cruiseSpeed);
            std::cout << "Air speed: " << simVars.asiAirspeed
                      << "\tVertical speed: " << simVars.vsiVerticalSpeed
                      << "\tRPM: " << simVars.rpmEngine << std::endl
                      << "FlapCnt: " << simVars.tfFlapsCount
                      << "|FlapIdx: " << simVars.tfFlapsIndex << std::endl;
            printf(
                "Trim deflection: %f rad, trim Indicator: %f, Transponder "
                "code: %X\n\n",
                simVars.tfElevatorTrim, simVars.tfElevatorTrimIndicator,
                int(simVars.transponderCode));
            displayDelay = 500;
          }
#endif  // DEBUG_VARS
          break;
        default:
          printf("Unknown request id: %ld\n", pObjData->dwRequestID);
          break;
      }       // Switch pObjData
      break;  // Case SIMOBJECT_DATA.
    }
    case SIMCONNECT_RECV_ID_QUIT:
      quit = true;
      break;
  }
}

void AddReadDefs() {
  for (int i = 0;; i++) {
    // End if all defs are added.
    if (SimVarDefs[i][0] == NULL) break;
    // Check type and add correct length.
    if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
      // Add string
      SIMCONNECT_DATATYPE dataType = SIMCONNECT_DATATYPE_STRING256;
      int dataLen = 256;
      if (strcmp(SimVarDefs[i][1], "string64") == 0) {
        dataType = SIMCONNECT_DATATYPE_STRING64;
        dataLen = 64;
      } else if (strcmp(SimVarDefs[i][1], "string8") == 0) {
        dataType = SIMCONNECT_DATATYPE_STRING8;
        dataLen = 8;
      }

      // Try to add data def.
      if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL,
                                         SimVarDefs[i][0], NULL,
                                         dataType) < 0) {
        printf("Data def failed: %s (string)\n", SimVarDefs[i][0]);
      } else {
        varSize += dataLen;
      }
    } else {
      // Add double
      if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL,
                                         SimVarDefs[i][0],
                                         SimVarDefs[i][1]) < 0) {
        printf("Data def failed: %s, %s\n", SimVarDefs[i][0], SimVarDefs[i][1]);
      } else {
        varSize += sizeof(double);
      }
    }
  }
}

void MapEvents() {
  for (int i = 0;; i++) {
    if (WriteEvents[i].name == NULL) {
      break;
    }

    if (SimConnect_MapClientEventToSimEvent(hSimConnect, WriteEvents[i].id,
                                            WriteEvents[i].name) != 0) {
      printf("Map event failed: %s\n", WriteEvents[i].name);
    }
  }
}

void SubscribeEvents() {
  // Request an event when the simulation starts
  if (SimConnect_SubscribeToSystemEvent(hSimConnect, SIM_START, "SimStart") <
      0) {
    printf("Subscribe event failed: SimStart\n");
  }

  if (SimConnect_SubscribeToSystemEvent(hSimConnect, SIM_STOP, "SimStop") < 0) {
    printf("Subscribe event failed: SimStop\n");
  }
}

void CleanUp() {
  if (hSimConnect) {
    if (SimConnect_RequestDataOnSimObject(
            hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER,
            SIMCONNECT_PERIOD_NEVER, 0, 0, 0, 0) < 0) {
      printf("Failed to stop requesting data\n");
    }

    printf("Disconnecting from MS FS2020\n");
    SimConnect_Close(hSimConnect);
  }
}

int Run(const std::string& inputComPort) {
  printf("Input com port: %s", inputComPort.c_str());
  printf("DataLink %s\n", versionString);
  printf("Searching for local MS FS2020...\n");

  std::unique_ptr<SerialPort> serial;
  
  if (!inputComPort.empty())
    serial.reset(new SerialPort(("\\\\.\\" + inputComPort).c_str()));
  simVars.connected = 0;
  char serialInputBuf[10];
  HRESULT result;

  int bytesRead = 0;
  int retryDelay = 0;
  double newTrim = 0;
  while (!quit) {
    if (simVars.connected) {
      result = SimConnect_CallDispatch(hSimConnect, MyDispatchProcRd, NULL);
      if (result != 0) {
        printf("Disconnected from MS FS2020\n");
        simVars.connected = 0;
        printf("Searching for local MS FS2020...\n");
      }
      if (serial && serial->isConnected()) {
        // Handle input from serial.
        bytesRead = serial->readSerialPort(serialInputBuf, 1);
        if (bytesRead > 0) {
          switch (serialInputBuf[0]) {
            case 1:
              // compute new trim pos(up).
              newTrim =
                  (int)((-simVars.tfElevatorTrimIndicator - kTrimStep) * 16383);
              if (SimConnect_TransmitClientEvent(
                      hSimConnect, 0, KEY_AXIS_ELEV_TRIM_SET, (DWORD)newTrim,
                      SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                      SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
                printf("Failed to transmit event: %d\n",
                       KEY_AXIS_ELEV_TRIM_SET);
              } else {
                printf("big trim up to: %f\n", newTrim);
              }
              break;
            case 2:
              // compute new trim pos (down).
              newTrim =
                  (int)((-simVars.tfElevatorTrimIndicator + kTrimStep) * 16383);
              if (SimConnect_TransmitClientEvent(
                      hSimConnect, 0, KEY_AXIS_ELEV_TRIM_SET, (DWORD)newTrim,
                      SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                      SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
                printf("Failed to transmit event: %d\n",
                       KEY_AXIS_ELEV_TRIM_SET);
              }
              break;
          }
        }
      }
    } else if (retryDelay > 0) {
      retryDelay--;
    } else {
      result =
          SimConnect_Open(&hSimConnect, "Instrument Data Link", NULL, 0, 0, 0);
      if (result == 0) {
        printf("Connected to MS FS2020\n");
        AddReadDefs();
        MapEvents();
        SubscribeEvents();
        simVars.connected = 1;

        // Start requesting data
        if (SimConnect_RequestDataOnSimObject(
                hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER,
                SIMCONNECT_PERIOD_VISUAL_FRAME, 0, 0, 0, 0) < 0) {
          printf("Failed to start requesting data\n");
        }
      } else {
        retryDelay = 200;
      }
    }  // if connected/elif retry>0/else(retry).

    Sleep(10);
  }

  CleanUp();
  return 0;
}

const SimVars* const Read() { return &simVars; }

}  // namespace datalink
}  // namespace flight_panel
