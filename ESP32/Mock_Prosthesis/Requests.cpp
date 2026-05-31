#include <Arduino.h>
#include <NimBLEDevice.h>
#include "SharedComVars.h"
#include "ModularFunctionsHandeling.h"
#include "Requests.h"

void SendNotifyToServer(char* msgStr, int msgTypeEnum, NimBLERemoteCharacteristic* pRemoteCharacteristic){
  int totalMsgNum = ceil(((float)strlen(msgStr))/((float)(MAX_MSG_LEN-1)));
  if (totalMsgNum>1){Serial.println("The message is too long, dividing into multiple sends");}
  for (int msgNum=1;msgNum<=totalMsgNum;msgNum++){
    uint8_t* msgBytes = strToByteMsg(msgTypeEnum, msgStr, msgNum, totalMsgNum);
    uint16_t len = sizeof(struct msgInterpeterStruct);
    printMsg((struct msgInterpeterStruct*)msgBytes);
    pRemoteCharacteristic->writeValue(msgBytes, len);
      //TO DO- error handling
    free(msgBytes);
  }
}

void SimulateGestureRun(char* msgStr, NimBLERemoteCharacteristic* pRemoteCharacteristic){
  delay(1500);
  callFunction(msgStr);
  Serial.printf("Done playing %s, sending acknoweldge", msgStr);
  SendNotifyToServer(msgStr, GEST_ANS, pRemoteCharacteristic);
}
