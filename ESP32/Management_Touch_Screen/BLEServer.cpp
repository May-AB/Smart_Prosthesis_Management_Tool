#include "BLEServer.h"
#include "ConfigParams.h"
#include "Requests.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <SharedComVars.h>
#include <SharedYamlParser.h>
#include <vector>

void BLENotifyTask(void *parameter) {
  Serial.println("BLE Notify Task is running...");
  while (1) {
    if (bleNotifyTaskHandle) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      Serial.println("Emergency button pressed! Sending BLE notification...");
      SendEmergencyReq("STOP MOVEMENT!", EMERGENCY_STOP, pCharacteristic);
    } else {
      break;
    }
  }
}

void SendStatusChangeReq(std::vector<int> sensorsToOn,
                         std::vector<int> sensorsToOff) {
  char msgToSend[MAX_MSG_LEN];
  int ind = 0;
  msgToSend[0] = '\0';

  for (size_t i = 0; i < sensorsToOn.size(); i++) {
    int written = snprintf(&msgToSend[ind], sizeof(msgToSend) - ind, "%d|1",
                           sensorsToOn[i]);
    if (written > 0 && ind + written < (int)sizeof(msgToSend)) {
      ind += written;
    }
    if (sensorsToOff.size() > 0 || i < sensorsToOn.size() - 1) {
      if (ind < (int)sizeof(msgToSend) - 1) {
        msgToSend[ind++] = '|';
        msgToSend[ind] = '\0';
      }
    }
  }

  for (size_t i = 0; i < sensorsToOff.size(); i++) {
    int written = snprintf(&msgToSend[ind], sizeof(msgToSend) - ind, "%d|0",
                           sensorsToOff[i]);
    if (written > 0 && ind + written < (int)sizeof(msgToSend)) {
      ind += written;
    }
    if (i < sensorsToOff.size() - 1) {
      if (ind < (int)sizeof(msgToSend) - 1) {
        msgToSend[ind++] = '|';
        msgToSend[ind] = '\0';
      }
    }
  }
  SendNotifyToClient(msgToSend, CHANGE_SENSOR_STATE_REQ, pCharacteristic);
}

void SendSensorParamChangeReq(int idSensorToChange,
                              std::vector<int> paramIdToChange,
                              std::vector<int> parametersToChange) {
  char msgToSend[MAX_MSG_LEN];
  int ind = 0;
  msgToSend[0] = '\0';

  for (size_t i = 0; i < paramIdToChange.size(); i++) {
    int written =
        snprintf(&msgToSend[ind], sizeof(msgToSend) - ind, "%d|%d|%d",
                 idSensorToChange, paramIdToChange[i], parametersToChange[i]);
    if (written > 0 && ind + written < (int)sizeof(msgToSend)) {
      ind += written;
    }
    // Add separator if not the last item
    if (i < paramIdToChange.size() - 1) {
      if (ind < (int)sizeof(msgToSend) - 1) {
        msgToSend[ind++] = '|';
        msgToSend[ind] = '\0';
      }
    }
  }
  SendNotifyToClient(msgToSend, CHANGE_SENSOR_PARAM_REQ, pCharacteristic);
}

void SendMotorParamChangeReq(int idMotorToChange,
                             std::vector<int> parametersToChange) {
  char msgToSend[MAX_MSG_LEN];
  int ind = 0;
  msgToSend[0] = '\0';

  for (size_t i = 0; i < parametersToChange.size(); i++) {
    int written = snprintf(&msgToSend[ind], sizeof(msgToSend) - ind, "%d|%d",
                           idMotorToChange, parametersToChange[i]);
    if (written > 0 && ind + written < (int)sizeof(msgToSend)) {
      ind += written;
    }
    // Add separator if not the last item
    if (i < parametersToChange.size() - 1) {
      if (ind < (int)sizeof(msgToSend) - 1) {
        msgToSend[ind++] = '|';
        msgToSend[ind] = '\0';
      }
    }
  }
  SendNotifyToClient(msgToSend, CHANGE_MOTOR_PARAM_REQ, pCharacteristic);
}

// Class to handle events on connection and discconection from client
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(BLEServer *pServer, NimBLEConnInfo &connInfo) override {
    Serial.println("Client connected!");
    if (isDemoYaml.test_and_set()) {
      hasClient.clear();
    } else {
      isDemoYaml.clear();
      hasClient.test_and_set();
      sendYamlRequest = true;
    }
  }

  void onDisconnect(BLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override {
    Serial.println("Client disconnected! Advertsing again");
    hasClient.clear();
    if (debugTab) {
      // Schedule deleteDebug() on the LVGL (UI) core — safe cross-core call
      lv_async_call((lv_async_cb_t)deleteDebug, NULL);
    }
    if (!welcomeScreenFlag) {
      if (!isDemoYaml.test_and_set()) {
        isDemoYaml.clear();
        notFinishUpdateSensors.clear();
        notRemoveBox.clear();
        lv_event_send(welcomeScreen, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_invalidate(welcomeScreen);
        if (bleNotifyTaskHandle) {
          vTaskDelete(bleNotifyTaskHandle);
          bleNotifyTaskHandle = NULL;
        }
      }
    }

    pServer->startAdvertising();
  }
};

// Test function for debugging purposes only, handles return button
void BLEReturnBTNtest() {
  struct msgInterpeterStruct msg_bytes;
  strToByteMsg(&msg_bytes, 0, "Testing return button");
  uint16_t len = sizeof(struct msgInterpeterStruct);
  // printMsg(&msg_bytes);
  pCharacteristic->setValue((uint8_t *)&msg_bytes, len);
  pCharacteristic->notify();
}

// Callback for receiving confirmations from the client
class MyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic,
               NimBLEConnInfo &connInfo) {
    const uint8_t *receivedData = pCharacteristic->getValue().data();
    // printByteArray(MSG_SIZE, receivedData); // print byte array for debuging
    struct msgInterpeterStruct *receivedDataStruct =
        (struct msgInterpeterStruct *)receivedData;
    switch (receivedDataStruct->reqType) {
    case CHANGE_SENSOR_STATE_ANS: {
      printMsg(receivedDataStruct);
      notFinishUpdateSensors.clear();
      break;
    }
    case EDIT_REQ: {
      // Add handling for EDIT_REQ here
      break;
    }
    case FUNC_REQ: {
      // Add handling for FUNC_REQ here
      break;
    }
    case READ_ANS: {
      char receivedMsg[MAX_MSG_LEN];
      int isMotor;
      int hardwareId;
      int hardwareValue = 0;
      strcpy(receivedMsg, receivedDataStruct->msg);
      char *tokenedMsg;
      tokenedMsg = strtok(receivedMsg, "|");
      int i = 0;
      while (tokenedMsg != NULL) {
        if (i == 0) {
          isMotor = atoi(tokenedMsg);
          Serial.printf("received real time data answer for %s.\n",
                        isMotor == 1 ? "motor" : "sensor");
          i++;
        } else if (i == 1) {
          hardwareId = atoi(tokenedMsg);
          i++;
        } else {
          hardwareValue = atoi(tokenedMsg);
          break;
        }
        tokenedMsg = strtok(NULL, "|");
      }

      // Schedule the chart update on the UI core — safe cross-core call
      scheduleChartUpdate(hardwareValue);
      break;
    }
    case EDIT_ANS: {
      // Add handling for EDIT_ANS here
      break;
    }
    case FUNC_ANS: {
      // Add handling for FUNC_ANS here
      break;
    }
    case YML_SENSOR_ANS: {
      // Check if the msg was received succesfully by comparing the msg length
      // and checksum to the desired values
      pointerToSensorBuff = &sensorsYamlBuffer;
      ReciveYAMLField(pointerToSensorBuff, *receivedDataStruct);
      if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
        isYmlSensorsReady = true;
        SendNotifyToClient("Please send Motors data", YML_MOTORS_REQ,
                           pCharacteristic);
      }

      break;
    }

    case YML_MOTORS_ANS: {
      pointerToMotorsBuff = &motorsYamlBuffer;
      ReciveYAMLField(pointerToMotorsBuff, *receivedDataStruct);
      if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
        isYmlMotorsReady = true;
        SendNotifyToClient("Please send functions data", YML_FUNC_REQ,
                           pCharacteristic);
      }
      break;
    }

    case YML_FUNC_ANS: {
      pointerToFuncBuff = &funcsYamlBuffer;
      ReciveYAMLField(pointerToFuncBuff, *receivedDataStruct);
      if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
        isYmlFunctionsReady = true;
        SendNotifyToClient("Please send general data", YML_GENERAL_REQ,
                           pCharacteristic);
      }
      break;
    }

    case YML_GENERAL_ANS: {
      pointerToGeneralBuff = &generalYamlBuffer;
      ReciveYAMLField(pointerToGeneralBuff, *receivedDataStruct);
      if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
        isYmlGeneralReady = true;
      }
      break;
    }
    case YAML_ANS: {

      break;
    }
    case GEST_ANS: {
      canPlayGesture.test_and_set();
      break;
    }
    case CHANGE_MOTOR_PARAM_ANS: {
      printMsg(receivedDataStruct);
      notFinishUpdateSensors.clear();
      break;
    }

    case CHANGE_SENSOR_PARAM_ANS: {
      printMsg(receivedDataStruct);
      notFinishUpdateSensors.clear();
      break;
    }
    default:
      break;
    }
  }
};

void sendingGesture(const char *gestureName) {
  struct msgInterpeterStruct byteMsg;
  strToByteMsg(&byteMsg, GEST_REQ, gestureName);
  // printByteArray(MSG_SIZE, (uint8_t*)&byteMsg);
  uint16_t len = MSG_SIZE;
  pCharacteristic->setValue((uint8_t *)&byteMsg, len);
  pCharacteristic->notify();
}

void StartBLEServer(void *params) {
  NimBLEDevice::init("");
  pServer = NimBLEDevice::createServer();
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pServer->setCallbacks(new ServerCallbacks());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  pAdvertising->setName("UIScreen");
  pAdvertising->addServiceUUID(NimBLEUUID(SERVICE_UUID));
  pAdvertising->start();

  Serial.println("Server is advertising");
  while (1) {

    delay(2000);
  }
}
