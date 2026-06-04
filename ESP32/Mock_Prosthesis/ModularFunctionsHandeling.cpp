#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SharedYamlParser.h>
#include "ModularFunctionsHandeling.h"

int currentSensorId = 0;
char* sensorStatus = nullptr;

// Simulating gestures functions. 
// this should be replaced by the function that controls the gesture on the prosthesis side.
void scissors(void) { printf("Run gesture scissors\n"); }
void rock(void) { printf("Run gesture rock\n"); }
void paper(void) { printf("Run gesture paper \n"); }
void rest(void) { printf("Returning to rest position\n"); }

// Simulating sensor state. 
// this should be replaced by the function that changes the sensor state on the prosthesis side.
void ChangeSensorState(int currentSensorId, const char* sensorStatus) { 
  Serial.printf("old status is %s.\n",sensors[currentSensorId].status.c_str());
  sensors[currentSensorId].status=strcmp(sensorStatus,"1")==0 ? "on" : "off";
  Serial.printf("new status is %s.\n",sensors[currentSensorId].status.c_str());
}

// Simulating motor and sensor values. 
// this should be replaced by the function that reads the motor and sensor values on the prosthesis side.
int SampleMotorsAndSensors(int isMotor, int hardwareId) { 
  int min;
  int max;
  if(isMotor){
    min = 5;
    max = 30;
  } else{
    min = 10;
    max = 90;
  }
  return (rand() % (max - min + 1) + min);
}

// Wrapper function for ChangeSensorState
void ChangeSensorStateWrapper(void) {
    ChangeSensorState(currentSensorId, sensorStatus);
}

// Function map
const static struct {
    const char *name;
    void (*func)(void);
} functionMap[] = {
    { "scissors", scissors },
    { "rock", rock },
    { "paper", paper },
    { "rest", rest },
    { "ChangeSensorState", ChangeSensorStateWrapper }, // Use wrapper function
};

// Function caller
int callFunction(const char *name) {
    for (size_t i = 0; i < (sizeof(functionMap) / sizeof(functionMap[0])); i++) {
        if (!strcmp(functionMap[i].name, name) && functionMap[i].func) {
            functionMap[i].func();
            return 0;
        }
    }
    return -1;
}
