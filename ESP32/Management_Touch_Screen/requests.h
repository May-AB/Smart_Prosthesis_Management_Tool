#include "HardwareSerial.h"
#include "Print.h"
#ifndef REQUESTS_H
#define REQUESTS_H

#include "SharedComVars.h"
#include "SharedYamlParser.h"

// Initialize YAML flags
extern bool isYmlGeneralReady;
extern bool isYmlSensorsReady;
extern bool isYmlMotorsReady;
extern bool isYmlFunctionsReady;

class NimBLECharacteristic;

void SendNotifyToClient(const char* msgStr, int msgTypeEnum, NimBLECharacteristic *pCharacteristic);
void SendEmergencyReq(const char* msgStr, int msgTypeEnum, NimBLECharacteristic *pCharacteristic);
bool isMsgCorrupted(struct msgInterpeterStruct* structVal);
void ReciveYAMLField(uint8_t** bufferToUse, struct msgInterpeterStruct structVal);

#endif //REQUESTS_H
