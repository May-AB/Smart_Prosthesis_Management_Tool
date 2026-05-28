#ifndef SHARED_COM_VALS_H
#define SHARED_COM_VALS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_MSG_LEN 128

enum msgTypeEnum{ 
  READ_REQ, EDIT_REQ, FUNC_REQ, YAML_REQ, GEST_REQ,
  READ_ANS, EDIT_ANS, FUNC_ANS, YAML_ANS, GEST_ANS,
  YML_SENSOR_REQ, YML_MOTORS_REQ, YML_FUNC_REQ, YML_GENERAL_REQ,
  YML_SENSOR_ANS, YML_MOTORS_ANS, YML_FUNC_ANS, YML_GENERAL_ANS,
  CHANGE_SENSOR_STATE_REQ, CHANGE_SENSOR_PARAM_REQ,
  CHANGE_SENSOR_STATE_ANS, CHANGE_SENSOR_PARAM_ANS,
  CHANGE_MOTOR_STATE_REQ, CHANGE_MOTOR_PARAM_REQ,
  CHANGE_MOTOR_STATE_ANS, CHANGE_MOTOR_PARAM_ANS,
  EMERGENCY_STOP
};

enum yamlFieldTypesEnum{ 
  SENSORS_FIELD, FUNCTIONS_FIELD, MOTORS_FIELD, GENERAL_FIELD
};

struct msgInterpeterStruct{
  int reqType;
  int curMsgCount;
  int totMsgCount;
  int msgLength;
  char msg[MAX_MSG_LEN];
  int checksum;
};

uint8_t calculateChecksum(const char* data, size_t length) {
    uint8_t checksum = 0;

    for (size_t i = 0; i < length; ++i) {
        checksum ^= data[i]; // XOR each byte
    }

    return checksum;
}

uint8_t* strToByteMsg(int reqType, char* msgStr, int msgNum = 1, int totalMsgNum = 1) {
  size_t struct_size = sizeof(struct msgInterpeterStruct);
  uint8_t* byte_msg = (uint8_t*)malloc(struct_size);
  if (byte_msg == NULL) {
      perror("Failed to allocate memory for MSG");
      return NULL;
  }
  struct msgInterpeterStruct *msg_buff = (struct msgInterpeterStruct *) byte_msg;
  msg_buff->curMsgCount = msgNum;  
  msg_buff->totMsgCount = totalMsgNum;  
  msg_buff->reqType = reqType;
  size_t start = (msgNum - 1) * (MAX_MSG_LEN - 1);
  size_t remainderToEnd = (strlen(msgStr) - start);
  size_t currentChunkSize = (MAX_MSG_LEN-1 < remainderToEnd) ? MAX_MSG_LEN-1 : remainderToEnd;
  strncpy(msg_buff->msg, msgStr + start, currentChunkSize);
  msg_buff->msg[currentChunkSize] = '\0';
  uint8_t checksum_result = calculateChecksum(msg_buff->msg, currentChunkSize);
  msg_buff->checksum= checksum_result;
  msg_buff->msgLength = currentChunkSize;
  // Copy the struct contents to the byte array
  return byte_msg;
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
#endif //SHARED_COM_VALS_H
