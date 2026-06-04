#include <Arduino.h>
#include "SharedComVars.h"

uint8_t calculateChecksum(const char* data, size_t length) {
    uint8_t checksum = 0;

    for (size_t i = 0; i < length; ++i) {
        checksum ^= data[i]; // XOR each byte
    }

    return checksum;
}

void ReciveMultipleMSGS(uint8_t** bufferToUse, struct msgInterpeterStruct structVal){
  Serial.printf("Recived msg %d out of %d.\n", structVal.curMsgCount, structVal.totMsgCount);
  if ((structVal.curMsgCount)==1) {
    *bufferToUse=(uint8_t*)calloc((structVal.totMsgCount)*MAX_MSG_LEN, sizeof(uint8_t));    
    if(!(*bufferToUse)){
      Serial.println("calloc failed");
    }
  } 
  memcpy( (*bufferToUse) + ((structVal.curMsgCount-1)*(MAX_MSG_LEN-1)) , structVal.msg,  structVal.msgLength );
}

void strToByteMsg(struct msgInterpeterStruct* msgBuff, int reqType, const char* msgStr, int msgNum, int totalMsgNum) {
  if (msgBuff == nullptr) return;
  msgBuff->curMsgCount = msgNum;  
  msgBuff->totMsgCount = totalMsgNum;  
  msgBuff->reqType = reqType;
  size_t start = (msgNum-1) * (MAX_MSG_LEN-1);
  size_t remainderToEnd = (strlen(msgStr) - start);
  size_t currentChunkSize = (MAX_MSG_LEN-1 < remainderToEnd) ? MAX_MSG_LEN-1 : remainderToEnd;
  strncpy(msgBuff->msg, msgStr + start, currentChunkSize);
  msgBuff->msg[currentChunkSize] = '\0';
  uint8_t checksumResult = calculateChecksum(msgBuff->msg, currentChunkSize);
  msgBuff->checksum= checksumResult;
  msgBuff->msgLength = currentChunkSize;
}

void printByteArray(size_t length, const uint8_t* pData){
  Serial.print("Byte array: ");
  for (size_t i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);  // Prints each byte as hexadecimal
    Serial.print(" ");
  }
  Serial.println();
}

void printMsg(struct msgInterpeterStruct* msg){
  Serial.printf("reqType: %d, curMsgCount: %d, totMsgCount: %d, msgLength: %d, msg: %s, desired checksum: %d\n",
  msg->reqType, msg->curMsgCount, msg->totMsgCount, msg->msgLength, msg->msg, msg->checksum);
}
