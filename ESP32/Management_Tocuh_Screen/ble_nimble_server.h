#ifndef BLE_NIMBLE_SERVER_H
#define BLE_NIMBLE_SERVER_H
#include "esp32-hal.h"
#include <NimBLEDevice.h>
#include <string>
#include <Arduino.h>
#include <vector>
#include "shared_com_vars.h"
#include "shared_yaml_parser.h"
#include "requests.h"
#include <atomic>

static NimBLECharacteristic *pCharacteristic;
static lv_obj_t* debugTab = NULL;

static bool confirmationReceived = false;
static bool sendYamlRequest = false;
static bool welcomeScreenFlag = true;
std::atomic_flag hasClient = ATOMIC_FLAG_INIT;
std::atomic_flag canPlayGesture = ATOMIC_FLAG_INIT;
std::atomic_flag isDemoYaml = ATOMIC_FLAG_INIT;
std::atomic_flag notRemoveBox = ATOMIC_FLAG_INIT;
std::atomic_flag notFinishUpdateSensors = ATOMIC_FLAG_INIT;

lv_obj_t* homeTab = NULL;
lv_obj_t* statTab = NULL;
lv_obj_t* setupTab = NULL;

lv_obj_t *welcomeScreen  = NULL; //

static lv_event_code_t EVENT_SENSOR_CHANGED_SECC = (lv_event_code_t)lv_event_register_id();
static lv_obj_t *mainUIScreen = NULL;
static bool mainFirstTime = true;

static lv_obj_t *chart;
static lv_chart_series_t *ser;
static lv_timer_t *chart_timer;
static lv_obj_t *close_chart_btn;
lv_obj_t* dropdownMotorsObj;
lv_obj_t* dropdownSensorsObj;
lv_obj_t* TabviewObjDebugMode;
lv_obj_t* title_label_bug;

QueueHandle_t buttonQueue;  // Global queue handle
TaskHandle_t bleNotifyTaskHandle = NULL;

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

// UUIDs for the service and characteristics
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
#define MSG_SIZE (sizeof(struct msgInterpeterStruct))
static NimBLEServer *pServer;

void SendStatusChangeReq(std::vector<int> sensors_to_on, std::vector<int> sensors_to_off){
  char* msg_to_send=(char*)malloc(MAX_MSG_LEN);
  int pos = 0;
  if (msg_to_send!=NULL) {

    for (int i=0; i<sensors_to_on.size(); i++){
      String sensor_id =String(sensors_to_on[i]);

      strcpy(&(msg_to_send[pos]), sensor_id.c_str());
      pos+=sensor_id.length();
      strcpy(&msg_to_send[pos++],"|");
      strcpy(&msg_to_send[pos++],"1");
      if (sensors_to_off.size()>0 || i<sensors_to_on.size()-1) {
        strcpy(&msg_to_send[pos++],"|");
      }
    }

    for (int i=0; i<sensors_to_off.size(); i++){
      String sensor_id =String(sensors_to_off[i]);
      strcpy(&(msg_to_send[pos]), sensor_id.c_str());
      pos+=sensor_id.length();
      strcpy(&(msg_to_send[pos++]),"|");
      strcpy(&(msg_to_send[pos++]),"0");
      if (i<sensors_to_off.size()-1) {
        strcpy(&(msg_to_send[pos++]),"|");
      }
    }

    SendNotifyToClient(msg_to_send, CHANGE_SENSOR_STATE_REQ, pCharacteristic);
    free(msg_to_send);
  }
}

void SendSensorParamChangeReq(
    int idSensorToChange,
    std::vector<int> paramIdToChange,
    std::vector<int> parametersToChange)
{
  char* msg_to_send=(char*)malloc(MAX_MSG_LEN);
  
  int pos = 0;
  if (msg_to_send!=NULL) {
    for (int i=0; i<paramIdToChange.size(); i++){
      String sensor_id =String(idSensorToChange);
      String param_id =String(paramIdToChange[i]);
      String parameter= String(parametersToChange[i]);
      
      // Copy sensor ID
      strcpy(&msg_to_send[pos], sensor_id.c_str());
      pos += sensor_id.length();
      // Separator
      msg_to_send[pos++] = '|';
      // Copy parameter ID
      strcpy(&msg_to_send[pos], param_id.c_str());
      pos += param_id.length();
      // Separator
      msg_to_send[pos++] = '|';
      // Copy parameter
      strcpy(&msg_to_send[pos], parameter.c_str());
      pos += parameter.length();
      // Add separator if not the last item
      if (i < paramIdToChange.size() - 1) {
          msg_to_send[pos++] = '|';
      }
    }
    SendNotifyToClient(msg_to_send, CHANGE_SENSOR_PARAM_REQ , pCharacteristic);
    free(msg_to_send);
  }
}

void SendMotorParamChangeReq(int id_motor_to_change, std::vector<int> parameters_to_change){
  char* msg_to_send=(char*)malloc(MAX_MSG_LEN);
  int pos = 0;
  if (msg_to_send!=NULL) {
    for (int i=0; i<parameters_to_change.size(); i++){
      String motor_id =String(id_motor_to_change);
      String parameter= String(parameters_to_change[i]);

      // Copy motor ID
      strcpy(&msg_to_send[pos], motor_id.c_str());
      pos += motor_id.length();
      // Separator
      msg_to_send[pos++] = '|';
      // Copy parameter
      strcpy(&msg_to_send[pos], parameter.c_str());
      pos += parameter.length();
      // Add separator if not the last item
      if (i < parameters_to_change.size() - 1) {
          msg_to_send[pos++] = '|';
      }
    }
    SendNotifyToClient(msg_to_send, CHANGE_MOTOR_PARAM_REQ , pCharacteristic);
    free(msg_to_send);
  }
}
void deleteDebug(){
  if (chart_timer) {
    lv_timer_del(chart_timer); // Stop the timer
    chart_timer = NULL;
  }
  delay(200);
  if(title_label_bug){
    lv_obj_del(title_label_bug);
    title_label_bug = NULL;
  }
  if (chart) {// Delete the chart
    lv_obj_del(chart);
    chart = NULL;
  }
  if (close_chart_btn) {// Delete the close button
    lv_obj_del(close_chart_btn);
    close_chart_btn = NULL;
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
    const uint8_t* received_data = pCharacteristic->getValue().data();
    //printByteArray(MSG_SIZE, received_data); // print byte array for debuging
    struct msgInterpeterStruct* received_data_struct = (struct msgInterpeterStruct*)received_data;
    switch (received_data_struct->reqType) {
      case CHANGE_SENSOR_STATE_ANS:{
        printMsg(received_data_struct);
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
        char* received_msg= (char*)malloc(MAX_MSG_LEN);
        int is_motor;
        int hardware_id;
        int hardware_value;
        if (received_msg){
          strcpy(received_msg,received_data_struct->msg);
          char* tokened_msg ;
          tokened_msg=strtok(received_msg, "|");
          int i=0;
          while(tokened_msg != NULL) {
            if (i==0){
              is_motor=atoi(tokened_msg);
              Serial.printf("received real time data answer for %s.\n",
              is_motor==1 ? "motor" : "sensor");
              i++;
            } 
            else if (i==1) {
                hardware_id = atoi(tokened_msg);
                i++;
            } else {
              hardware_value = atoi(tokened_msg);
              break;
            }
          tokened_msg = strtok(NULL, "|");
          }
          if (received_msg) free(received_msg);

          lv_chart_set_next_value(chart, ser, hardware_value); // Update chart

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
        pointer_to_sensor_buff = &sensors_yaml_buffer;
        ReciveYAMLField(pointer_to_sensor_buff,*received_data_struct);
        if (received_data_struct->curMsgCount == received_data_struct->totMsgCount) {
          isYmlSensorsReady = true;
          SendNotifyToClient("Please send Motors data", YML_MOTORS_REQ, pCharacteristic);
        }

        break;

      case YML_MOTORS_ANS:
        pointer_to_motors_buff = &motors_yaml_buffer;
        ReciveYAMLField(pointer_to_motors_buff,*received_data_struct);
        if (received_data_struct->curMsgCount == received_data_struct->totMsgCount) {
          isYmlMotorsReady = true;
          SendNotifyToClient( "Please send functions data", YML_FUNC_REQ, pCharacteristic );
        }
        break;

      case YML_FUNC_ANS:
          pointer_to_func_buff = &funcs_yaml_buffer;
          ReciveYAMLField(pointer_to_func_buff,*received_data_struct);
          if (received_data_struct->curMsgCount == received_data_struct->totMsgCount) {
            isYmlFunctionsReady = true;
            SendNotifyToClient( "Please send general data", YML_GENERAL_REQ, pCharacteristic );
          }
          break;     

      case YML_GENERAL_ANS:
          pointer_to_general_buff = &general_yaml_buffer;
          ReciveYAMLField(pointer_to_general_buff,*received_data_struct);
          if (received_data_struct->curMsgCount == received_data_struct->totMsgCount) {
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
      printMsg(received_data_struct);
      notFinishUpdateSensors.clear();
      break;
      }
	  	
	  case CHANGE_SENSOR_PARAM_ANS:
	    {
      printMsg(received_data_struct);
      notFinishUpdateSensors.clear();
      break;
      }
    default:
        break;
    }
  }
};

void sendingGesture(char* gestureName) {
  uint8_t* byte_msg = strToByteMsg(GEST_REQ, gestureName);
  // printByteArray(MSG_SIZE,byte_msg);
  uint16_t len = MSG_SIZE; 
  pCharacteristic->setValue(byte_msg, len);
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
#endif //BLE_NIMBLE_SERVER_H
