#include <Arduino.h>
#include <NimBLEDevice.h>

#include "SharedComVars.h"
#include "Requests.h"
#include "CreateYamlFile.h"
#include "SharedYamlParser.h"
#include "ModularFunctionsHandeling.h"

static const NimBLEAdvertisedDevice* advDevice;
static bool                          doConnect  = false;
static uint32_t                      scanTimeMs = 5000; /** scan time in milliseconds, 0 = scan forever */
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override { Serial.printf("Connected\n"); }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs, false, false);
    }

    /********************* Security handled here *********************/
    void onPassKeyEntry(NimBLEConnInfo& connInfo) override {
        Serial.printf("Server Passkey Entry\n");
        /**
         * This should prompt the user to enter the passkey displayed
         * on the peer device.
         */
        NimBLEDevice::injectPassKey(connInfo, 123456);
    }

    void onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pass_key) override {
        Serial.printf("The passkey YES/NO number: %" PRIu32 "\n", pass_key);
        /** Inject false if passkeys don't match. */
        NimBLEDevice::injectConfirmPasskey(connInfo, true);
    }

    /** Pairing process complete, we can check the results in connInfo */
    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if (!connInfo.isEncrypted()) {
            Serial.printf("Encrypt connection failed - disconnecting\n");
            /** Find the client with the connection handle provided in connInfo */
            NimBLEDevice::getClientByHandle(connInfo.getConnHandle())->disconnect();
            return;
        }
    }
} clientCallbacks;

/** Define a class to handle the callbacks when scan events are received */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        Serial.printf("Address: %s\n", advertisedDevice->getAddress().toString().c_str());        
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))){
          Serial.printf("Found Our Service\n");
          /** stop scan before connecting */
          NimBLEDevice::getScan()->stop();
          /** Save the device reference in a global for the client to use*/
          advDevice = advertisedDevice;
          /** Ready to connect now */
          doConnect = true;
        }
    }

    /** Callback to process the results of the completed scan or restart it */
    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.printf("Scan Ended, reason: %d, device count: %d; Restarting scan\n", reason, results.getCount());
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} scanCallbacks;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    std::string str  = (isNotify == true) ? "Notification" : "Indication";
    Serial.println("received request");
    struct msgInterpeterStruct receivedDataVal = *((struct msgInterpeterStruct*)pData);
    struct msgInterpeterStruct* receivedData = &receivedDataVal;
    printMsg(receivedData);
    switch (receivedData->reqType) {
      case EMERGENCY_STOP:
        Serial.println("Recived emergency stop request");
        callFunction("EmergencyStop");
        break;
    
    case GEST_REQ:
      SimulateGestureRun(receivedData->msg, pRemoteCharacteristic);
      break;
    
    case YAML_REQ:
      Serial.println("Recivied yaml request, sending sensors data");
      {
        char* sensorsSplitedField2 = nullptr;
        splitYaml(readYAML().c_str(), NULL, &sensorsSplitedField2, NULL, NULL);
        if (sensorsSplitedField2) {
          SendNotifyToServer(sensorsSplitedField2, YML_SENSOR_ANS, pRemoteCharacteristic);
          free(sensorsSplitedField2);
        }
      }
      break;

    case YML_MOTORS_REQ:
      // Sending motors data
      {
        char* motorsSplitedField2 = nullptr;
        splitYaml(readYAML().c_str(), NULL, NULL, &motorsSplitedField2, NULL);
        if (motorsSplitedField2) {
          SendNotifyToServer(motorsSplitedField2, YML_MOTORS_ANS, pRemoteCharacteristic);
          free(motorsSplitedField2);
        }
      }
      break;

    case YML_FUNC_REQ:
      // Sending functions data
      {
        char* functionsSplitedField2 = nullptr;
        splitYaml(readYAML().c_str(), NULL, NULL, NULL, &functionsSplitedField2);
        if (functionsSplitedField2) {
          SendNotifyToServer(functionsSplitedField2, YML_FUNC_ANS, pRemoteCharacteristic);
          free(functionsSplitedField2);
        }
      }
      break;

    case YML_GENERAL_REQ:
      // Sending general data
      {
        char* generalSplitedField2 = nullptr;
        splitYaml(readYAML().c_str(), &generalSplitedField2, NULL, NULL, NULL);
        if (generalSplitedField2) {
          SendNotifyToServer(generalSplitedField2, YML_GENERAL_ANS, pRemoteCharacteristic);
          free(generalSplitedField2);
        }
        Serial.println("Finished sending yaml data");
      }
      break;

    case CHANGE_SENSOR_STATE_REQ:
      // Handling requests to change sensor status on <=> off
      {
        char receivedMsg[MAX_MSG_LEN * 4];
        if (receivedData->totMsgCount > 4) {
          Serial.println("Error: totMsgCount exceeds stack buffer capacity");
          break;
        }
        strcpy(receivedMsg, receivedData->msg);
        char* tokenedMsg;
        tokenedMsg = strtok(receivedMsg, "|");
        int i = 0;
        while (tokenedMsg != NULL) {
          if (i == 2 || i == 0) {
            i = 0;
            currentSensorId = atoi(tokenedMsg);
            Serial.printf("New sensor ID is %d, sensor name is %s\n", currentSensorId, sensors[currentSensorId].name.c_str());
          } else {
            sensorStatus = tokenedMsg;
            //Serial.printf("new state is %s.\n", sensorStatus);
            callFunction("ChangeSensorState");
          }
          tokenedMsg = strtok(NULL, "|");
          i++;
        }

        SendNotifyToServer(receivedData->msg, CHANGE_SENSOR_STATE_ANS, pRemoteCharacteristic);
      }
      break;

    case CHANGE_SENSOR_PARAM_REQ:
      {
        char receivedMsg[MAX_MSG_LEN * 4];
        if (receivedData->totMsgCount > 4) {
          Serial.println("Error: totMsgCount exceeds stack buffer capacity");
          break;
        }
        strcpy(receivedMsg, receivedData->msg);
        char* tokenedMsg;
        tokenedMsg = strtok(receivedMsg, "|");
        int i = 0;
        int parameterId;
        while (tokenedMsg != NULL) {
          if (i == 3 || i == 0) {
            i = 0;
            currentSensorId = atoi(tokenedMsg);
            Serial.printf("New sensor ID is %d, sensor name is %s.\n", currentSensorId, sensors[currentSensorId].name.c_str());
          } else if (i == 1) {
            parameterId = atoi(tokenedMsg);
          } else {
            int newParameterVal = atoi(tokenedMsg);
            // Finding the corresponding map key inside the struct
            std::map<String, Parameter> parameterMap = sensors[currentSensorId].function.parameters;
            int j = 0;
            for (const auto& [paramName, param] : parameterMap) {
              if (j == parameterId) {
                int maxVal = parameterMap[paramName].max;
                int minVal = parameterMap[paramName].min;
                if ((newParameterVal <= maxVal) && (newParameterVal >= minVal) && (parameterMap[paramName].modifyPermission==true)) {
                  parameterMap[paramName].currentVal = newParameterVal;
                  Serial.printf("New val %d for key %s in sensor ID %d\n", newParameterVal, paramName, currentSensorId);
                }
                else{
                  Serial.printf("New val is not in range or nor permitted\n");
                }
                break;
              }
              j++;
            }
          }
          tokenedMsg = strtok(NULL, "|");
          i++;
        }
        SendNotifyToServer(receivedData->msg, CHANGE_SENSOR_PARAM_ANS, pRemoteCharacteristic);
      }
      break;
    
    case CHANGE_MOTOR_PARAM_REQ:
      {
        char receivedMsg[MAX_MSG_LEN * 4];
        if (receivedData->totMsgCount > 4) {
          Serial.println("Error: totMsgCount exceeds stack buffer capacity");
          break;
        }
        int currentMotorId;
        strcpy(receivedMsg, receivedData->msg);
        char* tokenedMsg ;
        tokenedMsg=strtok(receivedMsg, "|");
        int i=0;
        int parameterId;
        while(tokenedMsg != NULL) {
          if (i==2|| i==0){
            i=0;
            currentMotorId=atoi(tokenedMsg);
            Serial.printf("new motor id is %d, motor name is %s.\n",currentMotorId,motors[currentMotorId].name.c_str());
          } 
          else {
            int  newParameterVal = atoi(tokenedMsg);
            // Finding the corrosponding map  key in inside the struct
            int j=0;
            int maxVal =   motors[currentMotorId].safetyThreshold.max;
            int minVal =   motors[currentMotorId].safetyThreshold.min;
            if (( newParameterVal <= maxVal ) && ( newParameterVal >= minVal ) && (motors[currentMotorId].safetyThreshold.modifyPermission==true)) {
              motors[currentMotorId].safetyThreshold.currentVal=newParameterVal;
              Serial.printf("new safety threshold is %d for motors id %d\n",newParameterVal, currentMotorId);
            }
            else {
              Serial.printf( "Parameter cant be changed! allowed range: [%d, %d], modification permission: %s\n" , 
                motors[currentMotorId].safetyThreshold.min,  motors[currentMotorId].safetyThreshold.max,
                (motors[currentMotorId].safetyThreshold.modifyPermission)? "true" :"false");
            }
          }
          tokenedMsg = strtok(NULL, "|");
          i++;
        }
        SendNotifyToServer(receivedData->msg, CHANGE_MOTOR_PARAM_ANS, pRemoteCharacteristic);
      }
      break;

    case READ_REQ:{
      char receivedMsg[MAX_MSG_LEN];
      int isMotor;
      int hardwareId;
      strcpy(receivedMsg,receivedData->msg);
      char* tokenedMsg ;
      tokenedMsg=strtok(receivedMsg, "|");
      int i=0;
      while(tokenedMsg != NULL) {
        if (i==0){
          isMotor=atoi(tokenedMsg);
          Serial.printf("received real time data request for %s.\n",
          isMotor==1 ? "motor" : "sensor");
          i++;
        } 
        else {
          hardwareId = atoi(tokenedMsg);
          break;
        }
        tokenedMsg = strtok(NULL, "|");
      }
      int sampledData=SampleMotorsAndSensors(isMotor, hardwareId);
      String sampledDataStr = String(sampledData);
      Serial.printf("sampled data str %s. msg length %d \n",sampledDataStr.c_str(),receivedData->msgLength);
      strcpy(receivedMsg,receivedData->msg);
      strcpy(&(receivedMsg[receivedData->msgLength]),"|");
      strcpy(&(receivedMsg[receivedData->msgLength+1]),sampledDataStr.c_str());
      SendNotifyToServer(receivedMsg, READ_ANS, pRemoteCharacteristic); // Send response
      Serial.printf("msg: %s\n",receivedMsg);
      break;}

    default:
        Serial.println("unrecognized response");
        break;
    } 
}



/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {
    NimBLEClient* pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getCreatedClientCount()) {
        /**
         *  Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient) {
            if (!pClient->connect(advDevice, false)) {
                Serial.printf("Reconnect failed\n");
                return false;
            }
            Serial.printf("Reconnected client\n");
        } else {
            /**
             *  We don't already have a client that knows this device,
             *  check for a client that is disconnected that we can use.
             */
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if (!pClient) {
        if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.printf("Max clients reached - no more connections available\n");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.printf("New client created\n");

        pClient->setClientCallbacks(&clientCallbacks, false);
        /**
         *  Set initial connection parameters:
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
         */
        pClient->setConnectionParams(12, 12, 0, 150);

        /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
        pClient->setConnectTimeout(5 * 1000);

        if (!pClient->connect(advDevice)) {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            Serial.printf("Failed to connect, deleted client\n");
            return false;
        }
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Serial.printf("Failed to connect\n");
            return false;
        }
    }

    Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

    /** Now we can read/write/subscribe the characteristics of the services we are interested in */
    NimBLERemoteService*        pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;

    pSvc = pClient->getService(SERVICE_UUID);
    if (pSvc) {
        pChr = pSvc->getCharacteristic(CHARACTERISTIC_UUID);
    }

    if (pChr) {
        if (pChr->canRead()) {
            Serial.printf("%s Value: %s\n", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
        }

        if (pChr->canWrite()) {
            if (pChr->writeValue("")) {
            } else {
                pClient->disconnect();
                return false;
            }

            if (pChr->canRead()) {
                Serial.printf("The value is now: %s\n", pChr->readValue().c_str());
            }
        }

        if (pChr->canNotify()) {
            if (!pChr->subscribe(true, notifyCB)) {
                pClient->disconnect();
                return false;
            }
        } else if (pChr->canIndicate()) {
            /** Send false as first argument to subscribe to indications instead of notifications */
            if (!pChr->subscribe(false, notifyCB)) {
                pClient->disconnect();
                return false;
            }
        }
    } else {
        Serial.printf("service not found.\n");
    }

    pSvc = pClient->getService(SERVICE_UUID);
    if (pSvc) {
        pChr = pSvc->getCharacteristic(CHARACTERISTIC_UUID);
        if (pChr) {
            if (pChr->canRead()) {
                Serial.printf("Value: %s\n", pChr->readValue().c_str());
            }

            if (pChr->canWrite()) {
                if (pChr->writeValue("")) {
                } else {
                    pClient->disconnect();
                    return false;
                }

                if (pChr->canRead()) {
                    
                }
            }

            if (pChr->canNotify()) {
                if (!pChr->subscribe(true, notifyCB)) {
                    pClient->disconnect();
                    return false;
                }
            } else if (pChr->canIndicate()) {
                /** Send false as first argument to subscribe to indications instead of notifications */
                if (!pChr->subscribe(false, notifyCB)) {
                    pClient->disconnect();
                    return false;
                }
            }
        }
    } else {
        Serial.printf("service not found.\n");
    }

    Serial.printf("Done with this device!\n");
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.printf("Starting NimBLE Client\n");
    initYaml();
    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Client");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&scanCallbacks, false);
    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(100);
    pScan->setWindow(100);
    /** Start scanning for advertisers */
    pScan->start(scanTimeMs);
    Serial.printf("Scanning for peripherals\n");
}

void loop() {
  /** Loop here until we find a device we want to connect to */
  delay(100);
  if (doConnect) {
    doConnect = false;
    /** Found a device we want to connect to, do it now */
    if (connectToServer()) {
        Serial.printf("Success! we should now be getting notifications, scanning for more!\n");
    } else {
        Serial.printf("Failed to connect, starting scan\n");
        NimBLEDevice::getScan()->start(scanTimeMs, false, false);      
    }
  }
}