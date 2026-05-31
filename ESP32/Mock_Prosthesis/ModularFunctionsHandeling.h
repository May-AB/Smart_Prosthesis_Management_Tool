#ifndef ModularFunctionsHandeling_H
#define ModularFunctionsHandeling_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "SharedYamlParser.h"
 
extern int currentSensorId;
extern char* sensorStatus;

// Simulating gestures functions
void scissors();
void rock();
void paper();
void rest();

// Simulating sensor state
void ChangeSensorState(int currentSensorId, const char* sensorStatus);
int GetRealTimeData(int isMotor, int hardwareId);
void ChangeSensorStateWrapper();

// Function caller
int callFunction(const char *name);

#endif //ModularFunctionsHandeling_H