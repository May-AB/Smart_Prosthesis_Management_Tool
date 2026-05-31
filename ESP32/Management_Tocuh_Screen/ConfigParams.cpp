#include "ConfigParams.h"

/* Display configuration instantiations */
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    2 /* DC */,
    15 /* CS */,
    14 /* SCK */,
    13 /* MOSI */,
    GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, -1 /* RST */, 3 /* rotation */, true /* IPS */);

uint32_t screenWidth = 0;
uint32_t screenHeight = 0;
lv_disp_draw_buf_t drawBuf;
lv_color_t *dispDrawBuf = nullptr;
lv_disp_drv_t disp_drv;
lv_obj_t *tabview = nullptr;
lv_obj_t* techTab = nullptr;

bool isUser = false;
bool isTech = false;
bool yamlStructsReady = false;
bool initialUserScreenFlag = false;
bool hasUnsavedChanges = false;
bool currIsSetup = false;
std::vector<lv_obj_t*> sensorSwitchVec;
lv_obj_t* sendNewSwitchesMsgBox = nullptr;
lv_obj_t* saveBtn = nullptr;
lv_obj_t* saveBtnTechSensors = nullptr;
lv_obj_t* saveBtnTechMotors = nullptr;
lv_obj_t * meter = nullptr;

lv_obj_t* msgBoxParent = nullptr;
std::vector<lv_obj_t*> objsToDeleteSensors;
std::vector<lv_obj_t*> objsToDeleteMotors;
int currentEditSensorId = -1;
int currentEditMotorId = -1;

std::vector<lv_obj_t*> currentEditSensorSlidersVec;
std::vector<lv_obj_t*> currentEditMotorSlidersVec;

uint16_t currTechTabviewId = 4;
lv_obj_t* techTabview = nullptr;
lv_obj_t * dropdownMotors = nullptr;
lv_obj_t * dropdownSensors = nullptr;
lv_obj_t * dropdownMotorsActivity = nullptr;
lv_obj_t * techTabMotors = nullptr;
lv_obj_t * techTabSensors = nullptr;
lv_obj_t * techTabMotorsActivity = nullptr;
bool sensorTriggeredByTechTab = false;
bool motorTriggeredByTechTab = false;

int numPoints = 80;
lv_obj_t *showChartBtn = nullptr;

int techPass = 0;
int debugPass = 0; 

lv_obj_t *initialUserScreen = nullptr;
lv_obj_t *readYamlFromProtScreen = nullptr;
lv_obj_t* searchBleScreen = nullptr;
lv_obj_t *passwordScreen = nullptr;
lv_obj_t *textarea = nullptr;
lv_obj_t* msgCloseBtn = nullptr;

NimBLECharacteristic *pCharacteristic = nullptr;
lv_obj_t* debugTab = nullptr;

bool confirmationReceived = false;
bool sendYamlRequest = false;
bool welcomeScreenFlag = true;

std::atomic_flag hasClient = ATOMIC_FLAG_INIT;
std::atomic_flag canPlayGesture = ATOMIC_FLAG_INIT;
std::atomic_flag isDemoYaml = ATOMIC_FLAG_INIT;
std::atomic_flag notRemoveBox = ATOMIC_FLAG_INIT;
std::atomic_flag notFinishUpdateSensors = ATOMIC_FLAG_INIT;

lv_obj_t* homeTab = nullptr;
lv_obj_t* statTab = nullptr;
lv_obj_t* setupTab = nullptr;

lv_obj_t *welcomeScreen = nullptr;

lv_event_code_t EVENT_SENSOR_CHANGED_SECC; // Will be initialized dynamically in setup() or static initializer
static bool register_event_once = []() {
    EVENT_SENSOR_CHANGED_SECC = (lv_event_code_t)lv_event_register_id();
    return true;
}();

lv_obj_t *mainUIScreen = nullptr;
bool mainFirstTime = true;

lv_obj_t *chart = nullptr;
lv_chart_series_t *ser = nullptr;
lv_timer_t *chartTimer = nullptr;
lv_obj_t *closeChartBTN = nullptr;
lv_obj_t* dropdownMotorsObj = nullptr;
lv_obj_t* dropdownSensorsObj = nullptr;
lv_obj_t* TabviewObjDebugMode = nullptr;
lv_obj_t* debugLabel = nullptr;

QueueHandle_t buttonQueue = nullptr;
TaskHandle_t bleNotifyTaskHandle = nullptr;

NimBLEServer *pServer = nullptr;
