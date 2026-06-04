#include "esp32-hal.h"
#ifndef REQUESTS_H
#define REQUESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ModularFunctionsHandeling.h"


class NimBLERemoteCharacteristic;

void SendNotifyToServer(const char* msgStr, int msgTypeEnum, NimBLERemoteCharacteristic* pRemoteCharacteristic);
void SimulateGestureRun(const char* msgStr, NimBLERemoteCharacteristic* pRemoteCharacteristic);

#endif //REQUESTS_H