#ifndef CONFIGPARAMS_H
#define CONFIGPARAMS_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <NimBLEDevice.h>
#include <atomic>
#include <vector>

#define TFT_BL 27
#define GFX_BL DF_GFX_BL // default backlight pin

/* Display configuration declarations */
extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;

/* Touch include */
#define STACK_SIZE 8192

// define hex_colors
#define HEX_BLACK lv_color_hex(0x000000) // Black
#define HEX_WHITE lv_color_hex(0xffffff) // White
#define HEX_LIGHT_GRAY lv_color_hex(0xf4f4f4) // Light Gray
#define HEX_LIGHT_GRAY_2 lv_color_hex(0xdfdfdf) // Light Gray_2
#define HEX_MEDIUM_GRAY lv_color_hex(0xCACACA) // Medium Gray
#define HEX_DARK_GRAY lv_color_hex(0x404040) // Dark Gray
#define HEX_DARK_BLUE lv_color_hex(0x00008b) // Dark Blue
#define HEX_ROYAL_BLUE lv_color_hex(0x4169e1) // Royal Blue
#define HEX_SKY_BLUE lv_color_hex(0x00bfff) // Sky Blue
#define HEX_PINK lv_color_hex(0xffc0cb) // Pink
#define HEX_PURPLE lv_color_hex(0x800080) // Purple
#define HEX_RED lv_color_hex(0xc30a12) // Red
#define HEX_BABY_PINK lv_color_hex(0xf4c2c2) // Baby Pink
#define HEX_BURGUNDY lv_color_hex(0x800020) // Burgundy
#define HEX_GOLD lv_color_hex(0xffd700) // Gold
#define HEX_TURQUOISE lv_color_hex(0x40e0d0) // Turquoise
#define HEX_LIGHT_PURPLE lv_color_hex(0xda70d6) // Light Purple
#define HEX_MEDIUM_PURPLE lv_color_hex(0x9370db) // Medium Purple
#define HEX_DARK_PURPLE lv_color_hex(0x663399) // Dark Purple
#define HEX_YELLOW lv_color_hex(0xf7edbe) // Bannana yellow
#define HEX_GREEN lv_color_hex(0x047a04) // Green

/* define GT911 touch screen parameters */
 #define TOUCH_GT911
 #define TOUCH_GT911_SCL 32
 #define TOUCH_GT911_SDA 33
 #define TOUCH_GT911_INT -1
 #define TOUCH_GT911_RST 25
 #define TOUCH_GT911_ROTATION ROTATION_RIGHT//ROTATION_NORMAL
 #define TOUCH_MAP_X1 320
 #define TOUCH_MAP_X2 0
 #define TOUCH_MAP_Y1 240
 #define TOUCH_MAP_Y2 0
 
/* Change to your screen resolution */
extern uint32_t screenWidth;
extern uint32_t screenHeight;
extern lv_disp_draw_buf_t drawBuf;
extern lv_color_t *dispDrawBuf;
extern lv_disp_drv_t disp_drv;
extern lv_obj_t *tabview;  // Declare the global tabview variable
extern lv_obj_t* techTab;

extern bool isUser;
extern bool isTech;
extern bool yamlStructsReady;
extern bool initialUserScreenFlag;
extern bool hasUnsavedChanges;
extern bool currIsSetup;
extern std::vector<lv_obj_t*> sensorSwitchVec;
extern lv_obj_t* sendNewSwitchesMsgBox;
extern lv_obj_t* saveBtn;
extern lv_obj_t* saveBtnTechSensors;
extern lv_obj_t* saveBtnTechMotors;
extern lv_obj_t * meter;

extern lv_obj_t* msgBoxParent;
extern std::vector<lv_obj_t*> objsToDeleteSensors;
extern std::vector<lv_obj_t*> objsToDeleteMotors;
extern int currentEditSensorId;
extern int currentEditMotorId;

extern std::vector<lv_obj_t*> currentEditSensorSlidersVec;
extern std::vector<lv_obj_t*> currentEditMotorSlidersVec;

extern uint16_t currTechTabviewId;
extern lv_obj_t* techTabview;
extern lv_obj_t * dropdownMotors;
extern lv_obj_t * dropdownSensors;
extern lv_obj_t * dropdownMotorsActivity;
extern lv_obj_t * techTabMotors;
extern lv_obj_t * techTabSensors;
extern lv_obj_t * techTabMotorsActivity;
extern bool sensorTriggeredByTechTab;
extern bool motorTriggeredByTechTab;

extern int numPoints; // Number of points in the chart
extern lv_obj_t *showChartBtn;

extern int techPass;
extern int debugPass; 

extern lv_obj_t *initialUserScreen; //
extern lv_obj_t *readYamlFromProtScreen;
extern lv_obj_t* searchBleScreen;
extern lv_obj_t *passwordScreen;
extern lv_obj_t *textarea;
extern lv_obj_t* msgCloseBtn;

extern NimBLECharacteristic *pCharacteristic;
extern lv_obj_t* debugTab;

extern bool confirmationReceived;
extern bool sendYamlRequest;
extern bool welcomeScreenFlag;

extern std::atomic_flag hasClient;
extern std::atomic_flag canPlayGesture;
extern std::atomic_flag isDemoYaml;
extern std::atomic_flag notRemoveBox;
extern std::atomic_flag notFinishUpdateSensors;

extern lv_obj_t* homeTab;
extern lv_obj_t* statTab;
extern lv_obj_t* setupTab;

extern lv_obj_t *welcomeScreen; //

extern lv_event_code_t EVENT_SENSOR_CHANGED_SECC;
extern lv_obj_t *mainUIScreen;
extern bool mainFirstTime;

extern lv_obj_t *chart;
extern lv_chart_series_t *ser;
extern lv_timer_t *chartTimer;
extern lv_obj_t *closeChartBTN;
extern lv_obj_t* dropdownMotorsObj;
extern lv_obj_t* dropdownSensorsObj;
extern lv_obj_t* TabviewObjDebugMode;
extern lv_obj_t* debugLabel;

extern QueueHandle_t buttonQueue;  // Global queue handle
extern TaskHandle_t bleNotifyTaskHandle;

// UUIDs for the service and characteristics
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
#define MSG_SIZE (sizeof(struct msgInterpeterStruct))
extern NimBLEServer *pServer;

extern char selectedTextToTitle[32];

#endif // CONFIGPARAMS_H