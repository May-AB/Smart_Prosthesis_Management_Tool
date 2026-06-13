#ifndef REQUESTS_H
#define REQUESTS_H

#include <SharedComVars.h>
#include <SharedYamlParser.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Queue used by the BLE core to hand assembled YAML section buffers to the
// LVGL core for parsing. BLE core allocates + enqueues; LVGL core dequeues,
enum class YamlSection : uint8_t { SENSORS, MOTORS, FUNCTIONS, GENERAL };
struct YamlSectionMsg {
  YamlSection section; // which section this buffer belongs to
  uint8_t *buf;        // heap-allocated buffer (BLE calloc'd, LVGL frees)
};
extern QueueHandle_t yamlSectionQueue;

// Queue used by the LVGL core to post outgoing BLE notifications to the BLE
// core.
struct BLENotifyMsg {
  char msg[MAX_MSG_LEN];
  int msgTypeEnum;
};
extern QueueHandle_t bleNotifySendQueue;

class NimBLECharacteristic;

void SendNotifyToClient(const char *msgStr, int msgTypeEnum,
                        NimBLECharacteristic *pCharacteristic);
void SendEmergencyReq(const char *msgStr, int msgTypeEnum,
                      NimBLECharacteristic *pCharacteristic);
bool isMsgCorrupted(struct msgInterpeterStruct *structVal);
void ReciveYAMLField(uint8_t **bufferToUse,
                     struct msgInterpeterStruct structVal);

#endif // REQUESTS_H
