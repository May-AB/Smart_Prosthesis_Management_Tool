#include <Arduino.h>
#include "SharedComVars.h"

uint8_t calculateChecksum(const char* data, size_t length) {
    uint8_t checksum = 0;

    for (size_t i = 0; i < length; ++i) {
        checksum ^= data[i]; // XOR each byte
    }

    return checksum;
}

void strToByteMsg(struct msgInterpeterStruct* msgBuff, int reqType, const char* msgStr, int msgNum, int totalMsgNum) {
  if (msgBuff == nullptr) return;
  msgBuff->curMsgCount = msgNum;  
  msgBuff->totMsgCount = totalMsgNum;  
  msgBuff->reqType = reqType;
  size_t start = (msgNum - 1) * (MAX_MSG_LEN - 1);
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
  Serial.printf(
      "msg: %s, reqType: %d, curMsgCount: %d, totMsgCount: %d, msgLength: %d, desired checksum: %d\n",
      msg->msg,
      msg->reqType,
      msg->curMsgCount,
      msg->totMsgCount,
      msg->msgLength,
      msg->checksum);
}
