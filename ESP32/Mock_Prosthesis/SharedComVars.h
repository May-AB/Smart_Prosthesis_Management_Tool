#ifndef SHARED_COM_VALS_H
#define SHARED_COM_VALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>


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


uint8_t calculateChecksum(const char* data, size_t length);
void ReciveMultipleMSGS(uint8_t** bufferToUse, struct msgInterpeterStruct structVal);
uint8_t* strToByteMsg(int reqType, const char* msgStr, int msgNum = 1, int totalMsgNum = 1);
void printByteArray(size_t length, const uint8_t* pData);
void printMsg(struct msgInterpeterStruct* msg);


#endif //SHARED_COM_VALS_H