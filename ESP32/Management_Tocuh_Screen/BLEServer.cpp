#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include "SharedComVars.h"
#include "SharedYamlParser.h"
#include "Requests.h"
#include "ConfigParams.h"
#include "BLEServer.h"

void BLENotifyTask(void *parameter) {
    Serial.println("BLE Notify Task is running...");
    while (1) {
        if (bleNotifyTaskHandle){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Serial.println("Emerency Button pressed! Sending BLE notification...");
        SendEmergencyReq("STOP MOVEMENT!", EMERGENCY_STOP, pCharacteristic);
        } else {
          break;
        }
    }
}

void SendStatusChangeReq(std::vector<int> sensorsToOn, std::vector<int> sensorsToOff){
  char* msgToSend=(char*)malloc(MAX_MSG_LEN);
  int ind = 0;
  if (msgToSend!=NULL) {

    for (size_t i=0; i<sensorsToOn.size(); i++){
      String sensorID =String(sensorsToOn[i]);

      strcpy(&(msgToSend[ind]), sensorID.c_str());
      ind+=sensorID.length();
      strcpy(&msgToSend[ind++],"|");
      strcpy(&msgToSend[ind++],"1");
      if (sensorsToOff.size()>0 || i<sensorsToOn.size()-1) {
        strcpy(&msgToSend[ind++],"|");
      }
    }

    for (size_t i=0; i<sensorsToOff.size(); i++){
      String sensorID =String(sensorsToOff[i]);
      strcpy(&(msgToSend[ind]), sensorID.c_str());
      ind+=sensorID.length();
      strcpy(&(msgToSend[ind++]),"|");
      strcpy(&(msgToSend[ind++]),"0");
      if (i<sensorsToOff.size()-1) {
        strcpy(&(msgToSend[ind++]),"|");
      }
    }

    SendNotifyToClient(msgToSend, CHANGE_SENSOR_STATE_REQ, pCharacteristic);
    free(msgToSend);
  }
}

void SendSensorParamChangeReq(
    int idSensorToChange,
    std::vector<int> paramIdToChange,
    std::vector<int> parametersToChange)
{
  char* msgToSend=(char*)malloc(MAX_MSG_LEN);
  
  int ind = 0;
  if (msgToSend!=NULL) {
    for (size_t i=0; i<paramIdToChange.size(); i++){
      String sensorID =String(idSensorToChange);
      String paramID =String(paramIdToChange[i]);
      String parameter= String(parametersToChange[i]);
      // Copy sensor ID
      strcpy(&msgToSend[ind], sensorID.c_str());
      ind += sensorID.length();
      // Separator
      msgToSend[ind++] = '|';
      // Copy parameter ID
      strcpy(&msgToSend[ind], paramID.c_str());
      ind += paramID.length();
      // Separator
      msgToSend[ind++] = '|';
      // Copy parameter
      strcpy(&msgToSend[ind], parameter.c_str());
      ind += parameter.length();
      // Add separator if not the last item
      if (i < paramIdToChange.size() - 1) {
          msgToSend[ind++] = '|';
      }
    }
    SendNotifyToClient(msgToSend, CHANGE_SENSOR_PARAM_REQ , pCharacteristic);
    free(msgToSend);
  }
}

void SendMotorParamChangeReq(int idMotorToChange, std::vector<int> parametersToChange){
  char* msgToSend=(char*)malloc(MAX_MSG_LEN);
  int ind = 0;
  if (msgToSend!=NULL) {
    for (size_t i=0; i<parametersToChange.size(); i++){
      String motorID =String(idMotorToChange);
      String parameter= String(parametersToChange[i]);

      // Copy motor ID
      strcpy(&msgToSend[ind], motorID.c_str());
      ind += motorID.length();
      // Separator
      msgToSend[ind++] = '|';
      // Copy parameter
      strcpy(&msgToSend[ind], parameter.c_str());
      ind += parameter.length();
      // Add separator if not the last item
      if (i < parametersToChange.size() - 1) {
          msgToSend[ind++] = '|';
      }
    }
    SendNotifyToClient(msgToSend, CHANGE_MOTOR_PARAM_REQ , pCharacteristic);
    free(msgToSend);
  }
}

//delete debug screens objects
void deleteDebug(){
  if (chartTimer) {
    lv_timer_del(chartTimer); // Stop the timer
    chartTimer = NULL;
  }
  delay(200);
  if(debugLabel){
    lv_obj_del(debugLabel);
    debugLabel = NULL;
  }
  if (chart) {// Delete the chart
    lv_obj_del(chart);
    chart = NULL;
  }
  if (closeChartBTN) {// Delete the close button
    lv_obj_del(closeChartBTN);
    closeChartBTN = NULL;
  }
  if(dropdownMotorsObj){
    lv_obj_del(dropdownMotorsObj);
    dropdownMotorsObj = NULL;
  }
  if(dropdownSensorsObj){
    lv_obj_del(dropdownSensorsObj);
    dropdownSensorsObj = NULL;
  }
  if (TabviewObjDebugMode){
    lv_obj_del(TabviewObjDebugMode);
    TabviewObjDebugMode = NULL;
  }
  if(debugTab){
    lv_obj_del(debugTab);
    debugTab = NULL;
  }
}

// Class to handle events on connection and discconection from client
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(BLEServer* pServer, NimBLEConnInfo & 	connInfo) override {
        Serial.println("Client connected!");
        if (isDemoYaml.test_and_set()) {
          hasClient.clear();
        }
        else{
          isDemoYaml.clear();
          hasClient.test_and_set();
          sendYamlRequest = true;
        }
    }

    void onDisconnect(BLEServer* pServer, NimBLEConnInfo & 	connInfo, int reason) override {
        Serial.println("Client disconnected! Advertsing again");
        hasClient.clear();
        if (debugTab) {
           deleteDebug();
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
              bleNotifyTaskHandle=NULL;
            }
          }
        }

        pServer->startAdvertising();
    }
};

// Test function for debugging purposes only, handles return button
void BLEReturnBTNtest(){
  uint8_t* msg_bytes = strToByteMsg(0, "Testing return button");
  uint16_t len = sizeof(struct msgInterpeterStruct);
  // printMsg((struct msgInterpeterStruct*)msg_bytes);
  pCharacteristic->setValue(msg_bytes, len);
  pCharacteristic->notify();
  free(msg_bytes);
}

// Callback for receiving confirmations from the client
class MyCallbacks: public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
    const uint8_t* receivedData = pCharacteristic->getValue().data();
    //printByteArray(MSG_SIZE, receivedData); // print byte array for debuging
    struct msgInterpeterStruct* receivedDataStruct = (struct msgInterpeterStruct*)receivedData;
    switch (receivedDataStruct->reqType) {
      case CHANGE_SENSOR_STATE_ANS:{
        printMsg(receivedDataStruct);
        notFinishUpdateSensors.clear();
        break;}
      case EDIT_REQ:
          // Add handling for EDIT_REQ here
          break;
      case FUNC_REQ:
          // Add handling for FUNC_REQ here
          break;
      case READ_ANS:
      {       
        char* receivedMsg= (char*)malloc(MAX_MSG_LEN);
        int isMotor;
        int hardwareId;
        int hardwareValue;
        if (receivedMsg){
          strcpy(receivedMsg,receivedDataStruct->msg);
          char* tokenedMsg ;
          tokenedMsg=strtok(receivedMsg, "|");
          int i=0;
          while(tokenedMsg != NULL) {
            if (i==0){
              isMotor=atoi(tokenedMsg);
              Serial.printf("received real time data answer for %s.\n",
              isMotor==1 ? "motor" : "sensor");
              i++;
            } 
            else if (i==1) {
                hardwareId = atoi(tokenedMsg);
                i++;
            } else {
              hardwareValue = atoi(tokenedMsg);
              break;
            }
          tokenedMsg = strtok(NULL, "|");
          }
          if (receivedMsg) free(receivedMsg);

          lv_chart_set_next_value(chart, ser, hardwareValue); // Update chart

          // Create a gap by setting the next few points to LV_CHART_POINT_NONE
          uint16_t p = lv_chart_get_point_count(chart);
          uint16_t s = lv_chart_get_x_start_point(chart, ser);
          lv_coord_t *a = lv_chart_get_y_array(chart, ser);

          a[(s + 1) % p] = LV_CHART_POINT_NONE;
          a[(s + 2) % p] = LV_CHART_POINT_NONE;
          a[(s + 3) % p] = LV_CHART_POINT_NONE;
          a[(s + 4) % p] = LV_CHART_POINT_NONE;
          a[(s + 5) % p] = LV_CHART_POINT_NONE;
          a[(s + 6) % p] = LV_CHART_POINT_NONE;
          a[(s + 7) % p] = LV_CHART_POINT_NONE;
          a[(s + 8) % p] = LV_CHART_POINT_NONE;
          a[(s + 9) % p] = LV_CHART_POINT_NONE;

          lv_chart_refresh(chart);
          }
    

          break;
          }
      case EDIT_ANS:
          // Add handling for EDIT_ANS here
          break;
      case FUNC_ANS:
          // Add handling for FUNC_ANS here
          break;
      case YML_SENSOR_ANS:
        //Check if the msg was received succesfully by comparing the msg length and checksum to the desired values 
        pointerToSensorBuff = &sensorsYamlBuffer;
        ReciveYAMLField(pointerToSensorBuff,*receivedDataStruct);
        if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
          isYmlSensorsReady = true;
          SendNotifyToClient("Please send Motors data", YML_MOTORS_REQ, pCharacteristic);
        }

        break;

      case YML_MOTORS_ANS:
        pointerToMotorsBuff = &motorsYamlBuffer;
        ReciveYAMLField(pointerToMotorsBuff,*receivedDataStruct);
        if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
          isYmlMotorsReady = true;
          SendNotifyToClient( "Please send functions data", YML_FUNC_REQ, pCharacteristic );
        }
        break;

      case YML_FUNC_ANS:
          pointerToFuncBuff = &funcsYamlBuffer;
          ReciveYAMLField(pointerToFuncBuff,*receivedDataStruct);
          if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
            isYmlFunctionsReady = true;
            SendNotifyToClient( "Please send general data", YML_GENERAL_REQ, pCharacteristic );
          }
          break;     

      case YML_GENERAL_ANS:
          pointerToGeneralBuff = &generalYamlBuffer;
          ReciveYAMLField(pointerToGeneralBuff,*receivedDataStruct);
          if (receivedDataStruct->curMsgCount == receivedDataStruct->totMsgCount) {
            isYmlGeneralReady = true;
          }
          break;     
      case YAML_ANS:
        
        break;
      case GEST_ANS:
        canPlayGesture.test_and_set();
        break;
	  case CHANGE_MOTOR_PARAM_ANS:
	    {
      printMsg(receivedDataStruct);
      notFinishUpdateSensors.clear();
      break;
      }
	  	
	  case CHANGE_SENSOR_PARAM_ANS:
	    {
      printMsg(receivedDataStruct);
      notFinishUpdateSensors.clear();
      break;
      }
    default:
        break;
    }
  }
};

void sendingGesture(char* gestureName) {
  uint8_t* byteMsg = strToByteMsg(GEST_REQ, gestureName);
  // printByteArray(MSG_SIZE,byteMsg);
  uint16_t len = MSG_SIZE; 
  pCharacteristic->setValue(byteMsg, len);
  pCharacteristic->notify();
}

void StartBLEServer(void* params) {
  NimBLEDevice::init("");
  pServer = NimBLEDevice::createServer();
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
                  );

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
