#include <Arduino.h>
#include <NimBLEDevice.h>
#include <SharedComVars.h>
#include "Requests.h"

// Define YAML flags
bool isYmlGeneralReady = false;
bool isYmlSensorsReady = false;
bool isYmlMotorsReady = false;
bool isYmlFunctionsReady = false;

void SendNotifyToClient(const char* msgStr, int msgTypeEnum, NimBLECharacteristic *pCharacteristic){
  int totalMsgNum = ceil(((float)strlen(msgStr))/((float)(MAX_MSG_LEN-1)));
  if (totalMsgNum>1){Serial.println("The message is too long, dividing into multiple sends");}
  for (int msgNum=1;msgNum<=totalMsgNum;msgNum++){
    struct msgInterpeterStruct msgBytes;
    strToByteMsg(&msgBytes, msgTypeEnum, msgStr, msgNum, totalMsgNum);
    uint16_t len = sizeof(struct msgInterpeterStruct);
    Serial.print("Sending msg:");
    printMsg(&msgBytes);
    pCharacteristic->setValue((uint8_t*)&msgBytes, len);
    pCharacteristic->notify();
      // TODO IN THE FUTURE - think if there is an possible error handling here. 
  }
}

// BOOT BUTTON
void SendEmergencyReq(const char* msgStr, int msgTypeEnum, NimBLECharacteristic *pCharacteristic){
    struct msgInterpeterStruct msgBytes;
    strToByteMsg(&msgBytes, msgTypeEnum, msgStr, 1, 1);
    Serial.print("Sending msg:");
    printMsg(&msgBytes);
    pCharacteristic->setValue((uint8_t*)&msgBytes, sizeof(struct msgInterpeterStruct));
    Serial.println("Notify to client");
    pCharacteristic->notify();
      // TODO IN THE FUTURE - think if there is an possible error handling here. 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Avoid spamming notifications
}

bool isMsgCorrupted(struct msgInterpeterStruct* structVal){
  if ((structVal->msgLength != strlen(structVal->msg)) ||
      calculateChecksum(structVal->msg, structVal->msgLength) != (structVal->checksum)) {
            Serial.printf("Got msg %d out of %d.\n", structVal->curMsgCount, structVal->totMsgCount);
            Serial.printf("Received MSG length and desired length are %s, (received length: %d, desired length: %d)\n",
                            structVal->msgLength == strlen(structVal->msg) ? "equal" : "not equal",
                            strlen(structVal->msg), structVal->msgLength);
            Serial.printf("Calculate MSG checksum is %s to desired cheksum!\n",
                (structVal->checksum) == (calculateChecksum(structVal->msg, structVal->msgLength))
                    ? "equal"
                    : "not equal");
    return true;
    }
  return false;
}

void ReciveYAMLField(uint8_t** bufferToUse, struct msgInterpeterStruct structVal){
  Serial.printf("Recived msg %d out of %d.\n", structVal.curMsgCount, structVal.totMsgCount);
  if ((structVal.curMsgCount) == 1) {
    *bufferToUse=(uint8_t*)calloc((structVal.totMsgCount) * MAX_MSG_LEN, sizeof(uint8_t));    
    if(!(*bufferToUse)){
      Serial.println("calloc failed");
    }
  } 
  memcpy(
      (*bufferToUse) + ((structVal.curMsgCount - 1) * (MAX_MSG_LEN - 1)),
      structVal.msg,
      structVal.msgLength);
}
