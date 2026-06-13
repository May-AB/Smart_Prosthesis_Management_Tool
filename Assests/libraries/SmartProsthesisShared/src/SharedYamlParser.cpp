#include "SharedYamlParser.h"
#include "SharedComVars.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <YAMLDuino.h>
#include <map>
#include <vector>

// Definition of split fields (for Mock Prosthesis and splits)
char *motorsSplitedField = nullptr;
char *generalSplitedField = nullptr;
char *sensorsSplitedField = nullptr;
char *functionsSplitedField = nullptr;

// Definition of Global vectors for parsed data
String fileType;
std::vector<General> generalEntries;
std::vector<Communication> communications;
std::vector<Sensor> sensors;
std::vector<Motor> motors;
std::vector<Function> functions;

// This function is used to define a new yaml string in order to replace the
// configuration yaml saved in SPIFF. To use the new yaml configuration, change
// the string below to your new yaml configuration and then uncomment the
// function call in the file CreateYamlFile.cpp and run it once to update the
// configuration yaml file.
const char *createDefaultYamlString() {
  const char *yamlContent = R"( /* insert yaml config here */
)";
  return yamlContent;
}

const char *createDemoYamlString() {
  const char *yamlContent = R"(
file_type: hand_system_configuration

general:
  - name: 'Technician_code'
    code: 28282
  - name: 'Debug_code'
    code: 2024

communications:
  - name: 'WiFi_server'
    status: 'off'
    ssid: 'user_HAND'
    password: 'Haifa3D'

  - name: 'BLE_client'
    status: 'on'
    mac: '11-11-11-11'
    SERVICE1_UUID: "6c09a8a9-be78-4596-9557-3c4bb4965058"
    CHARACTERISTIC1_UUID: "6c09a8a9-be78-4596-9558-3c4bb4965058"

sensors:
  - name: 'leg_pressure_sensor1'
    status: 'on'
    type: 'BLE_input'
    function:
      name: 'leg_function'
      parameters:
        param_1: [80,20,100,false]
        high_thld: [90,20,100,true]
        low_thld: [60,20,100,false]

  - name: 'shoulder_sensor1'
    status: 'off'
    type: 'BLE_input'
    function:
      name: 'shoulder_function'
      parameters:
        param_1: [60,20,100,true]
        high_thld: [30,20,100,true]
        low_thld: [80,20,100,true]

  - name: 'leg_pressure_sensor2'
    status: 'on'
    type: 'BLE_input'
    function:
      name: 'leg_function'
      parameters:
        param_1: [80,20,100,true]
        high_thld: [90,20,100,true]
        low_thld: [60,20,100,true]

  - name: 'shoulder_sensor2'
    status: 'off'
    type: 'BLE_input'
    function:
      name: 'shoulder_function'
      parameters:
        param_1: [60,20,100,true]
        high_thld: [30,20,100,true]
        low_thld: [80,20,100,true]

  - name: 'leg_pressure_sensor3'
    status: 'on'
    type: 'BLE_input'
    function:
      name: 'leg_function'
      parameters:
        param_1: [80,20,100,true]
        high_thld: [90,20,100,true]
        low_thld: [60,20,100,true]

  - name: 'shoulder_sensor3'
    status: 'off'
    type: 'BLE_input'
    function:
      name: 'shoulder_function'
      parameters:
        param_1: [60,20,100,true]
        high_thld: [30,20,100,true]
        low_thld: [80,20,100,true]

motors:
  - name: 'finger1_dc'
    type: 'DC_motor'
    pins:
      - type: 'in1_pin'
        pin_number: 19
      - type: 'in2_pin'
        pin_number: 21
      - type: 'sense_pin'
        pin_number: 34
    safety_threshold: [20,10,50,true]

  - name: 'finger2_dc'
    type: 'DC_motor'
    pins:
      - type: 'in1_pin'
        pin_number: 23
      - type: 'in2_pin'
        pin_number: 22
      - type: 'sense_pin'
        pin_number: 35
    safety_threshold: [20,10,50,false]

  - name: 'finge3_dc_blablabla'
    type: 'DC_motor'
    pins:
      - type: 'in1_pin'
        pin_number: 4
      - type: 'in2_pin'
        pin_number: 16
      - type: 'sense_pin'
        pin_number: 32
    safety_threshold: [20,10,50,true]

  - name: 'finge4_dc'
    type: 'DC_motor'
    pins:
      - type: 'in1_pin'
        pin_number: 18
      - type: 'in2_pin'
        pin_number: 17
      - type: 'sense_pin'
        pin_number: 33
    safety_threshold: [20,10,50,true]

  - name: 'turn_dc'
    type: 'DC_motor'
    pins:
      - type: 'in1_pin'
        pin_number: 26
      - type: 'in2_pin'
        pin_number: 27
      - type: 'sense_pin'
        pin_number: 36
    safety_threshold: [20,10,50,true]

functions:
  - name: 'send_debug_data'
    protocol_type: 'return_data'

  - name: 'run_motors'
    protocol_type: 'modify_only'

  - name: 'rock'
    protocol_type: 'gesture'
)";
  return yamlContent;
}

void splitYaml(const char *yaml, char **generalSplitedField,
               char **sensorsSplitedField, char **motorsSplitedField,
               char **functionsSplitedField) {
  Serial.println("recived yaml buffer, splitting...");
  const char *startGeneral = strstr(yaml, "general:");
  const char *startSensors = strstr(yaml, "sensors:");
  const char *startMotors = strstr(yaml, "motors:");
  const char *startFunctions = strstr(yaml, "functions:");

  // Find the end of each section by locating the next section or end of string
  const char *endGeneral =
      startSensors ? strstr(startSensors, "sensors:") : NULL;
  const char *endSensors = startMotors ? strstr(startMotors, "motors:") : NULL;
  const char *endMotors =
      startFunctions ? strstr(startFunctions, "functions:") : NULL;
  const char *endFunctions =
      NULL; // Functions section ends at the end of the string

  // Copy each section into the respective output strings
  if (startGeneral && generalSplitedField) {
    size_t len = (endGeneral ? endGeneral : yaml + strlen(yaml)) - startGeneral;
    *generalSplitedField = (char *)malloc((len + 1) * sizeof(char));
    strncpy(*generalSplitedField, startGeneral, len);
    (*generalSplitedField)[len] = '\0';
  }

  if (startSensors && sensorsSplitedField) {
    size_t len = (endSensors ? endSensors : yaml + strlen(yaml)) - startSensors;
    *sensorsSplitedField = (char *)malloc((len + 1) * sizeof(char));
    strncpy(*sensorsSplitedField, startSensors, len);
    (*sensorsSplitedField)[len] = '\0';
  }

  if (startMotors && motorsSplitedField) {
    size_t len = (endMotors ? endMotors : yaml + strlen(yaml)) - startMotors;
    *motorsSplitedField = (char *)malloc((len + 1) * sizeof(char));
    strncpy(*motorsSplitedField, startMotors, len);
    (*motorsSplitedField)[len] = '\0';
  }

  if (startFunctions && functionsSplitedField) {
    size_t len =
        (endFunctions ? endFunctions : yaml + strlen(yaml)) - startFunctions;
    *functionsSplitedField = (char *)malloc((len + 1) * sizeof(char));
    strncpy(*functionsSplitedField, startFunctions, len);
    (*functionsSplitedField)[len] = '\0';
  }
  Serial.println("yaml splited. ready for parsing");
}

void parseYAML(const int fieldType, const char *yamlContent) {
  JsonDocument doc;
  DeserializationError error = deserializeYml(doc, yamlContent);
  if (error) {
    Serial.print("Failed to parse YAML: ");
    Serial.println(error.f_str());
    return;
  }

  switch (fieldType) {
  case GENERAL_FIELD: {
    JsonArray generalArray = doc["general"];
    for (JsonObject entry : generalArray) {
      General gen;
      gen.name = entry["name"].as<String>();
      gen.code = entry["code"].as<int>();
      generalEntries.push_back(gen);
    }
    break;
  }
  case SENSORS_FIELD: {
    JsonArray sensorArray = doc["sensors"];
    for (JsonObject entry : sensorArray) {
      Sensor sensor;
      sensor.name = entry["name"].as<String>();
      sensor.status = entry["status"].as<String>();
      sensor.type = entry["type"].as<String>();

      JsonObject funcObj = entry["function"];
      sensor.function.name = funcObj["name"].as<String>();

      JsonObject params = funcObj["parameters"];
      for (JsonPair param : params) {
        Parameter paramData;
        JsonArray paramArray = param.value().as<JsonArray>();
        paramData.currentVal = paramArray[0];
        paramData.min = paramArray[1];
        paramData.max = paramArray[2];
        paramData.modifyPermission = paramArray[3];
        sensor.function.parameters[param.key().c_str()] = paramData;
      }
      sensors.push_back(sensor);
    }
    break;
  }
  case MOTORS_FIELD: {
    JsonArray motorArray = doc["motors"];
    for (JsonObject entry : motorArray) {
      Motor motor;
      motor.name = entry["name"].as<String>();
      motor.type = entry["type"].as<String>();

      JsonArray pinsArray = entry["pins"];
      for (JsonObject pin : pinsArray) {
        MotorPin motorPin;
        motorPin.type = pin["type"].as<String>();
        motorPin.pinNumber = pin["pin_number"].as<int>();
        motor.pins.push_back(motorPin);
      }

      JsonArray threshold = entry["safety_threshold"].as<JsonArray>();
      motor.safetyThreshold.currentVal = threshold[0];
      motor.safetyThreshold.min = threshold[1];
      motor.safetyThreshold.max = threshold[2];
      motor.safetyThreshold.modifyPermission = threshold[3];
      motors.push_back(motor);
    }
    break;
  }
  case FUNCTIONS_FIELD: {
    JsonArray actionArray = doc["functions"];
    for (JsonObject entry : actionArray) {
      Function function;
      function.name = entry["name"].as<String>();
      function.protocolType = entry["protocol_type"].as<String>();
      functions.push_back(function);
    }
    break;
  }
  default:
    Serial.println("Unknown field type!");
    break;
  }
}

void parseYAMLDemo(const String &yamlContent) {
  JsonDocument doc;
  DeserializationError error = deserializeYml(doc, yamlContent.c_str());
  if (error) {
    Serial.print("Failed to parse YAML: ");
    Serial.println(error.f_str());
    return;
  }

  fileType = doc["file_type"].as<String>();

  JsonArray generalArray = doc["general"];
  for (JsonObject entry : generalArray) {
    General gen;
    gen.name = entry["name"].as<String>();
    gen.code = entry["code"].as<int>();
    generalEntries.push_back(gen);
  }

  JsonArray commArray = doc["communications"];
  for (JsonObject entry : commArray) {
    Communication comm;
    comm.name = entry["name"].as<String>();
    comm.status = entry["status"].as<String>();
    comm.ssid = entry["ssid"].as<String>();
    comm.password = entry["password"].as<String>();
    comm.mac = entry["mac"].as<String>();
    comm.serviceUUID = entry["SERVICE1_UUID"].as<String>();
    comm.characteristicUUID = entry["CHARACTERISTIC1_UUID"].as<String>();
    communications.push_back(comm);
  }

  JsonArray sensorArray = doc["sensors"];
  for (JsonObject entry : sensorArray) {
    Sensor sensor;
    sensor.name = entry["name"].as<String>();
    sensor.status = entry["status"].as<String>();
    sensor.type = entry["type"].as<String>();

    JsonObject funcObj = entry["function"];
    sensor.function.name = funcObj["name"].as<String>();

    JsonObject params = funcObj["parameters"];
    for (JsonPair param : params) {
      Parameter paramData;
      JsonArray paramArray = param.value().as<JsonArray>();
      paramData.currentVal = paramArray[0];
      paramData.min = paramArray[1];
      paramData.max = paramArray[2];
      paramData.modifyPermission = paramArray[3];
      sensor.function.parameters[param.key().c_str()] = paramData;
    }
    sensors.push_back(sensor);
  }

  JsonArray motorArray = doc["motors"];
  for (JsonObject entry : motorArray) {
    Motor motor;
    motor.name = entry["name"].as<String>();
    motor.type = entry["type"].as<String>();

    JsonArray pinsArray = entry["pins"];
    for (JsonObject pin : pinsArray) {
      MotorPin motorPin;
      motorPin.type = pin["type"].as<String>();
      motorPin.pinNumber = pin["pin_number"].as<int>();
      motor.pins.push_back(motorPin);
    }

    JsonArray threshold = entry["safety_threshold"].as<JsonArray>();
    motor.safetyThreshold.currentVal = threshold[0];
    motor.safetyThreshold.min = threshold[1];
    motor.safetyThreshold.max = threshold[2];
    motor.safetyThreshold.modifyPermission = threshold[3];
    motors.push_back(motor);
  }

  JsonArray actionArray = doc["functions"];
  for (JsonObject entry : actionArray) {
    Function function;
    function.name = entry["name"].as<String>();
    function.protocolType = entry["protocol_type"].as<String>();
    functions.push_back(function);
  }
}

void printGeneral(const General &general) {
  Serial.println("General:");
  Serial.print("  Name: ");
  Serial.println(general.name);
  Serial.print("  Code: ");
  Serial.println(general.code);
}

void printParameter(const Parameter &parameter, const String &parameterName) {
  if (!parameterName.isEmpty()) {
    Serial.print("  Parameter Name: ");
    Serial.println(parameterName);
  }
  Serial.print("    Current Value: ");
  Serial.println(parameter.currentVal);
  Serial.print("    Min: ");
  Serial.println(parameter.min);
  Serial.print("    Max: ");
  Serial.println(parameter.max);
  Serial.print("    Modify Permission: ");
  Serial.println(parameter.modifyPermission ? "true" : "false");
}

void printSensorFunction(const SensorFunction &function) {
  Serial.println("  Function:");
  Serial.print("    Name: ");
  Serial.println(function.name);
  Serial.println("    Parameters:");
  for (const auto &[paramName, param] : function.parameters) {
    printParameter(param, paramName);
  }
}

void printSensor(const Sensor &sensor) {
  Serial.println("Sensor:");
  Serial.print("  Name: ");
  Serial.println(sensor.name);
  Serial.print("  Status: ");
  Serial.println(sensor.status);
  Serial.print("  Type: ");
  Serial.println(sensor.type);
  printSensorFunction(sensor.function);
}

void printMotorPin(const MotorPin &pin) {
  Serial.print("    Pin Type: ");
  Serial.println(pin.type);
  Serial.print("    Pin Number: ");
  Serial.println(pin.pinNumber);
}

void printMotor(const Motor &motor) {
  Serial.println("Motor:");
  Serial.print("  Name: ");
  Serial.println(motor.name);
  Serial.print("  Type: ");
  Serial.println(motor.type);
  Serial.println("  Pins:");
  for (const auto &pin : motor.pins) {
    printMotorPin(pin);
  }
  Serial.println("  Safety Threshold:");
  printParameter(motor.safetyThreshold);
}

void printFunction(const Function &function) {
  Serial.println("Function:");
  Serial.print("  Name: ");
  Serial.println(function.name);
  Serial.print("  Protocol Type: ");
  Serial.println(function.protocolType);
}

void splitSensorsField(const char *yaml) {
  Serial.println("splitting sensors");
  const char *sensorsStart = strstr(yaml, "sensors:");
  if (!sensorsStart) {
    Serial.println("No sensors section found.");
    return;
  }

  sensorsStart = strstr(sensorsStart, "- name:");

  while (sensorsStart && strstr(sensorsStart, "- name:")) {
    char *strTitle = "sensors:\n  ";

    const char *sensorsEnd = strstr(sensorsStart + 1, "- name:");
    if (sensorsEnd == nullptr) {
      sensorsEnd = yaml + strlen(yaml);
    }

    char *singleSensor = (char *)malloc(
        (sensorsEnd - sensorsStart + strlen(strTitle) + 1) * sizeof(char));
    if (singleSensor == nullptr) {
      Serial.println("Memory allocation failed.");
      return;
    }

    strncpy(singleSensor, strTitle, strlen(strTitle));
    strncpy(&singleSensor[strlen(strTitle)], sensorsStart,
            sensorsEnd - sensorsStart);
    singleSensor[sensorsEnd - sensorsStart + strlen(strTitle)] = '\0';

    parseYAML(SENSORS_FIELD, singleSensor);

    sensorsStart = sensorsEnd;
    free(singleSensor);
  }

  Serial.println("=== Sensors ===");
  for (const auto &sensor : sensors) {
    printSensor(sensor);
    Serial.println();
  }
}

void splitFunctionsField(const char *yaml) {
  Serial.println("splitting functions");
  const char *functionsStart = strstr(yaml, "functions:");
  if (!functionsStart) {
    Serial.println("No functions section found.");
    return;
  }

  functionsStart = strstr(functionsStart, "- name:");

  while (functionsStart && strstr(functionsStart, "- name:")) {
    char *strTitle = "functions:\n  ";

    const char *functionEnd = strstr(functionsStart + 1, "- name:");
    if (functionEnd == nullptr) {
      functionEnd = yaml + strlen(yaml);
    }

    char *singleFunction = (char *)malloc(
        (functionEnd - functionsStart + strlen(strTitle) + 1) * sizeof(char));
    if (singleFunction == nullptr) {
      Serial.println("Memory allocation failed.");
      return;
    }

    strncpy(singleFunction, strTitle, strlen(strTitle));
    strncpy(&singleFunction[strlen(strTitle)], functionsStart,
            functionEnd - functionsStart);
    singleFunction[functionEnd - functionsStart + strlen(strTitle)] = '\0';

    parseYAML(FUNCTIONS_FIELD, singleFunction);

    functionsStart = functionEnd;
    free(singleFunction);
  }

  Serial.println("=== Functions ===");
  for (const auto &function : functions) {
    printFunction(function);
    Serial.println();
  }
}

void splitGeneralField(const char *yaml) {
  Serial.println("splitting general");
  const char *generalStart = strstr(yaml, "general:");
  if (!generalStart) {
    Serial.println("No general section found.");
    return;
  }

  generalStart = strstr(generalStart, "- name:");

  while (generalStart && strstr(generalStart, "- name:")) {
    char *strTitle = "general:\n  ";

    const char *generalEnd = strstr(generalStart + 1, "- name:");
    if (generalEnd == nullptr) {
      generalEnd = yaml + strlen(yaml);
    }

    char *singleGeneral = (char *)malloc(
        (generalEnd - generalStart + strlen(strTitle) + 1) * sizeof(char));
    if (singleGeneral == nullptr) {
      Serial.println("Memory allocation failed.");
      return;
    }

    strncpy(singleGeneral, strTitle, strlen(strTitle));
    strncpy(&singleGeneral[strlen(strTitle)], generalStart,
            generalEnd - generalStart);
    singleGeneral[generalEnd - generalStart + strlen(strTitle)] = '\0';

    parseYAML(GENERAL_FIELD, singleGeneral);

    generalStart = generalEnd;
    free(singleGeneral);
  }
}

void splitMotorsField(const char *yaml) {
  Serial.println("splitting motors");
  const char *motorsStart = strstr(yaml, "motors:");
  if (!motorsStart) {
    Serial.println("No motors section found.");
    return;
  }

  motorsStart = strstr(motorsStart, "- name:");

  while (motorsStart && strstr(motorsStart, "- name:")) {
    char *strTitle = "motors:\n  ";

    const char *motorsEnd = strstr(motorsStart + 1, "- name:");
    if (motorsEnd == nullptr) {
      motorsEnd = yaml + strlen(yaml);
    }

    char *singleMotor = (char *)malloc(
        (motorsEnd - motorsStart + strlen(strTitle) + 1) * sizeof(char));
    if (singleMotor == nullptr) {
      Serial.println("Memory allocation failed.");
      return;
    }

    strncpy(singleMotor, strTitle, strlen(strTitle));
    strncpy(&singleMotor[strlen(strTitle)], motorsStart,
            motorsEnd - motorsStart);
    singleMotor[motorsEnd - motorsStart + strlen(strTitle)] = '\0';

    parseYAML(MOTORS_FIELD, singleMotor);

    motorsStart = motorsEnd;
    free(singleMotor);
  }

  Serial.println("=== Motors ===");
  for (const auto &motor : motors) {
    printMotor(motor);
    Serial.println();
  }
}

void initYaml(const char *yamlContent) {
  splitYaml(yamlContent, &generalSplitedField, &sensorsSplitedField,
            &motorsSplitedField, &functionsSplitedField);

  if (motorsSplitedField) {
    splitMotorsField(motorsSplitedField);
    free(motorsSplitedField);
    motorsSplitedField = nullptr;
  }
  if (sensorsSplitedField) {
    splitSensorsField(sensorsSplitedField);
    free(sensorsSplitedField);
    sensorsSplitedField = nullptr;
  }
  if (generalSplitedField) {
    splitGeneralField(generalSplitedField);
    free(generalSplitedField);
    generalSplitedField = nullptr;
  }
  if (functionsSplitedField) {
    splitFunctionsField(functionsSplitedField);
    free(functionsSplitedField);
    functionsSplitedField = nullptr;
  }
}
