#ifndef SHARED_YAML_PARSER_H
#define SHARED_YAML_PARSER_H

#include "SharedComVars.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <YAMLDuino.h>
#include <map>
#include <vector>

#define FUNC_TYPE_GESTURE "gesture"

// Split fields (used on Mock Prosthesis and management split flows)
extern char *motorsSplitedField;
extern char *generalSplitedField;
extern char *sensorsSplitedField;
extern char *functionsSplitedField;

// Communication struct
struct Communication {
  String name;
  String status;
  String ssid;
  String password;
  String mac;
  String serviceUUID;
  String characteristicUUID;
};

// General struct
struct General {
  String name;
  int code;
};

// Parameter struct
struct Parameter {
  int currentVal;
  int min;
  int max;
  bool modifyPermission;
};

// Function struct
struct SensorFunction {
  String name;
  std::map<String, Parameter> parameters; // Key: Parameter name
};

// Sensor struct
struct Sensor {
  String name;
  String status;
  String type;
  SensorFunction function;
};

// MotorPin struct
struct MotorPin {
  String type;
  int pinNumber;
};

// Motor struct
struct Motor {
  String name;
  String type;
  std::vector<MotorPin> pins;
  Parameter safetyThreshold;
};

// Function struct
struct Function {
  String name;
  String protocolType;
};

// Global vectors for parsed data
extern String fileType;
extern std::vector<General> generalEntries;
extern std::vector<Communication> communications;
extern std::vector<Sensor> sensors;
extern std::vector<Motor> motors;
extern std::vector<Function> functions;

// YAML Parser function declarations
const char *createDefaultYamlString();
const char *createDemoYamlString();
void splitYaml(const char *yaml, char **generalSplitedField = NULL,
               char **sensorsSplitedField = NULL,
               char **motorsSplitedField = NULL,
               char **functionsSplitedField = NULL);
void parseYAML(const int fieldType, const char *yamlContent);
void parseYAMLDemo(const String &yamlContent);
void printGeneral(const General &general);
void printParameter(const Parameter &parameter,
                    const String &parameterName = "");
void printSensorFunction(const SensorFunction &function);
void printSensor(const Sensor &sensor);
void printMotorPin(const MotorPin &pin);
void printMotor(const Motor &motor);
void printFunction(const Function &function);
void splitSensorsField(const char *yaml);
void splitFunctionsField(const char *yaml);
void splitGeneralField(const char *yaml);
void splitMotorsField(const char *yaml);

// Unified initialization function
void initYaml(const char *yamlContent);

#endif // SHARED_YAML_PARSER_H
