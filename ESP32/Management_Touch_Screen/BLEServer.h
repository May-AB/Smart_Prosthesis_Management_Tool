#ifndef BLE_NIMBLE_SERVER_H
#define BLE_NIMBLE_SERVER_H

#include "Requests.h"
#include "UIDebugMode.h"
#include <SharedComVars.h>
#include <SharedYamlParser.h>

#include <vector>

void BLENotifyTask(void *parameter);
void SendStatusChangeReq(std::vector<int> sensorsToOn,
                         std::vector<int> sensorsToOff);
void SendSensorParamChangeReq(int idSensorToChange,
                              std::vector<int> paramIdToChange,
                              std::vector<int> parametersToChange);
void SendMotorParamChangeReq(int idMotorToChange,
                             std::vector<int> parametersToChange);
void BLEReturnBTNtest();
void sendingGesture(const char *gestureName);
void StartBLEServer(void *params);
#endif // BLE_NIMBLE_SERVER_H
