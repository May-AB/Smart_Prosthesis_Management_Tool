#ifndef BLE_NIMBLE_SERVER_H
#define BLE_NIMBLE_SERVER_H

#include "SharedComVars.h"
#include "SharedYamlParser.h"
#include "Requests.h"

#include <vector>

void BLENotifyTask(void *parameter);
void SendStatusChangeReq(std::vector<int> sensorsToOn, std::vector<int> sensorsToOff);
void SendSensorParamChangeReq(
    int idSensorToChange,
    std::vector<int> paramIdToChange,
    std::vector<int> parametersToChange);
void SendMotorParamChangeReq(int idMotorToChange, std::vector<int> parametersToChange);
void deleteDebug();
void BLEReturnBTNtest();
void sendingGesture(char* gestureName);
void StartBLEServer(void* params);
#endif //BLE_NIMBLE_SERVER_H
