#include <Arduino.h>
#include <lvgl.h>
#include <atomic>
#include <Arduino_GFX_Library.h>
#include <vector>
#include <esp32-hal.h>

#include "ConfigParams.h"
#include "Touch.h"
#include "BLEServer.h"
#include "SharedYamlParser.h"
#include "UIShared.h"
#include "UIUserMode.h"
#include "UITechMode.h"
#include "UIDebugMode.h"
#include "UINavigation.h"

const int buttonPin = 0; // PIN NUMBER OF EMERGENCY BUTTON
volatile unsigned long lastPressTime = 0;  // Stores the last press time
const unsigned long debounceDelay = 1000;  // Min time between presses (ms)

// Non-blocking LVGL timer used for YAML poll loop — avoids recursive loadYamlStep calls
static lv_timer_t* s_yamlPollTimer = nullptr;

void IRAM_ATTR buttonPress() {
    unsigned long currentTime = millis();
    if (currentTime - lastPressTime < debounceDelay) {
        return;  // Ignore if pressed too quickly
    }
    lastPressTime = currentTime;
    if (bleNotifyTaskHandle == NULL) {
        ets_printf("ERROR: Notify Task Handle is NULL in ISR!\n");
        return;
    }
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(bleNotifyTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* Display flushing */
void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p){
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

/* Read touch points */
void myTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data){
  if (touchHasSignal())
  {
      if (touchTouched())
      {
          data->state = LV_INDEV_STATE_PR;
          /*Set the coordinates*/
          data->point.x = touchLastX;
          data->point.y = touchLastY;
      }
      else if (touchReleased())
      {
          data->state = LV_INDEV_STATE_REL;
      }
  }
  else
  {
      data->state = LV_INDEV_STATE_REL;
  }
}

void showPasswordScreen(lv_event_t *e) {
  // Load the new screen
  lv_scr_load_anim(passwordScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

void readYamlFromProtScreenFunction() {
    readYamlFromProtScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(readYamlFromProtScreen, HEX_WHITE, 0);

    lv_obj_t * label1 = lv_label_create(readYamlFromProtScreen);
    lv_label_set_recolor(label1, true);
    lv_label_set_text(label1, "#00007f Reading YAML File. \n Please Wait... #");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label1,&lv_font_montserrat_28,0);
    lv_obj_add_event_cb(readYamlFromProtScreen, loadYamlStep, LV_EVENT_CLICKED, NULL);
} 

void setupWelcomeScreen() {
    welcomeScreen = lv_obj_create(NULL);
    lv_scr_load_anim(welcomeScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    lv_obj_set_style_bg_color(welcomeScreen, HEX_WHITE, 0);

    lv_obj_t * label1 = lv_label_create(welcomeScreen);
    lv_label_set_recolor(label1, true);
    lv_label_set_text(label1, "#000099 S##000066 P##00007f M##0000b2 T#");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_font(label1,&lv_font_montserrat_48,0);

    lv_obj_t * label2 = lv_label_create(welcomeScreen);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label2, 150);
    lv_label_set_text(label2, "Smart Prosthesis' Management Tool");
    lv_obj_set_style_text_font(label2,&lv_font_montserrat_12,0);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label3 = lv_label_create(welcomeScreen);
    lv_label_set_text(label3, "Press and hold to start");
    lv_obj_set_style_text_font(label3,&lv_font_montserrat_16,0);
    lv_obj_align(label3, LV_ALIGN_CENTER, 0, 50);

    lv_obj_add_event_cb(welcomeScreen, changeToMainScreen, LV_EVENT_LONG_PRESSED, NULL);
}

void changeToMainScreen(lv_event_t * e) {
    lv_refr_now(NULL);

    objsToDeleteSensors.clear();
    objsToDeleteMotors.clear();
    currentEditSensorId = -1;
    currentEditMotorId = -1;

    currentEditSensorSlidersVec.clear();
    currentEditMotorSlidersVec.clear();

    welcomeScreenFlag = false;
    if (hasClient.test_and_set()) {
      lv_scr_load_anim(readYamlFromProtScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
    else {
      hasClient.clear();
      lv_scr_load_anim(searchBleScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

void setSearching(lv_event_t * e) {
  lv_obj_t * btn = lv_event_get_target(e);
  lv_obj_t * label = lv_obj_get_child(btn, 0);
  lv_label_set_text(label, "searching...");
}

void setSearchAgain(lv_event_t * e) {
  lv_obj_t * btn = lv_event_get_target(e);
  lv_obj_t * label = lv_obj_get_child(btn, 0);
  lv_label_set_text(label, "Search Again");
}

void searchBleAgainEvent(lv_event_t * e) {
    delay(2500);
    changeToMainScreen(e);
    if (sendYamlRequest) {
      lv_event_send(readYamlFromProtScreen, LV_EVENT_CLICKED , NULL);
    }
}

void startDemoEvent(lv_event_t * e) {
  isDemoYaml.test_and_set();
  sensors.clear();
  motors.clear();
  functions.clear();
  communications.clear();
  generalEntries.clear();
  fileType.clear();

  initDefaultYaml();
  setupInitialUserScreen();
}

void searchClientBLEScreen(){
    searchBleScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(searchBleScreen, HEX_WHITE, 0);

    lv_obj_t * label1 = lv_label_create(searchBleScreen);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label1, 250);
    lv_label_set_recolor(label1, true);
    lv_label_set_text(label1, "#00007f No Bluetooth connection found for the prosthesis#");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_text_font(label1,&lv_font_montserrat_20,0);

    lv_obj_t* searchBleBtn = createNewBtn(
        searchBleScreen,
        170,
        30,
        LV_ALIGN_CENTER,
        0,
        10,
        "Search Again",
        HEX_ROYAL_BLUE,
        HEX_BLACK);
    lv_obj_t* testExampleBtn = createNewBtn(
        searchBleScreen,
        170,
        30,
        LV_ALIGN_CENTER,
        0,
        50,
        "Start DEMO YAML",
        HEX_YELLOW,
        HEX_BLACK);

    lv_obj_add_event_cb(searchBleBtn, setSearching, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(searchBleBtn, searchBleAgainEvent, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(searchBleBtn, setSearchAgain, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(testExampleBtn, startDemoEvent, LV_EVENT_CLICKED , NULL);
}

void mainSelectClickEvent(lv_event_t* e) {
        uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
        const char* txt = lv_btnmatrix_get_btn_text(lv_event_get_target(e), id);

        if(!txt){
          return;
        }

        if (strcmp(txt, "User") == 0) {
          isUser = true;
          if (mainFirstTime) {
            mainFirstTime = false;
            sensorSwitchVec.clear();
            setupMainUI();
            lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
          }
          else{
            lv_obj_del(mainUIScreen);
            sensorSwitchVec.clear();
            setupMainUI();
            lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
          } 
        } else if (strcmp(txt, "Tech") == 0) {
          isTech = true;
          lv_scr_load_anim(passwordScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        } else if (strcmp(txt, "Debug") == 0) {
          lv_scr_load_anim(passwordScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        }
}

void passClickEvent(lv_event_t *e) {
      lv_obj_t *btn_matrix = lv_event_get_target(e);
      const char *btn_text = lv_btnmatrix_get_btn_text(btn_matrix, lv_btnmatrix_get_selected_btn(btn_matrix));

      if (btn_text) {
          if (strcmp(btn_text, "OK") == 0) {
              const char *password = lv_textarea_get_text(textarea);
              int password_int = atoi(password);

              if (isTech && password_int==techPass) {
                lv_textarea_set_text(textarea, "");
                if (mainFirstTime) {
                  mainFirstTime = false;
                  sensorSwitchVec.clear();
                  setupMainUI();
                  lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
                }
                else{
                  lv_obj_del(mainUIScreen);
                  sensorSwitchVec.clear();
                  setupMainUI();
                  lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
                } 
              } else if (!isTech && password_int == debugPass) {
                lv_textarea_set_text(textarea, "");
                if (mainFirstTime) {
                  mainFirstTime = false;
                  sensorSwitchVec.clear();
                  setupMainUI();
                  lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
                }
                else{
                  lv_obj_del(mainUIScreen);
                  sensorSwitchVec.clear();
                  setupMainUI();
                  lv_scr_load_anim(mainUIScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
                } 
              } 
              else {
                  isTech = false;
                  lv_textarea_set_text(textarea, "");
                  lv_scr_load_anim(initialUserScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
              }                
          } else if (strcmp(btn_text, "Cancel") == 0) {
              isTech = false;
              lv_textarea_set_text(textarea, "");
              lv_scr_load_anim(initialUserScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
          } else if (strcmp(btn_text, LV_SYMBOL_BACKSPACE) == 0) {
              lv_textarea_del_char(textarea);
          } else {
              // Append the pressed key to the textarea
              lv_textarea_add_text(textarea, btn_text);
          }
      }
}

void setupInitialUserScreen() {
  isUser = false;
  isTech = false;
  initialUserScreen = lv_obj_create(NULL);
  lv_scr_load_anim(initialUserScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
  lv_refr_now(NULL);

  lv_obj_t* label = lv_label_create(initialUserScreen);
  lv_label_set_text(label, "Select Mode:");
  lv_obj_align(label,LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_font(label,&lv_font_montserrat_18,0);

  // Create a button matrix for user mode selection
  static char* btnm_map[] = {"User", "\n", "Tech", "Debug", ""}; // The last element must be an empty string
  int num_pointer = sizeof(btnm_map) / sizeof(btnm_map[0]);

  static char** map_ptr = btnm_map;

  lv_obj_t* btnm = createNewMatrixBtnChooseOne(
      initialUserScreen,
      &map_ptr,
      num_pointer,
      LV_ALIGN_CENTER,
      0,
      20,
      HEX_SKY_BLUE,
      HEX_DARK_BLUE,
      HEX_WHITE);

  for (const auto& user : generalEntries) {
      if ((strcmp(user.name.c_str(),"Technician_code"))==0){
        techPass = user.code;
      }
      else if ((strcmp(user.name.c_str(),"Debug_code"))==0) {
        debugPass = user.code;
      }
  } 

   // Create a new screen
  passwordScreen = lv_obj_create(NULL); // NULL creates a new screen
  lv_obj_set_style_bg_color(passwordScreen, HEX_LIGHT_GRAY, 0); // Optional: Set background color

  // Add a label
  lv_obj_t *label_pass = lv_label_create(passwordScreen);
  lv_label_set_recolor(label_pass, true);            
  lv_label_set_text(label_pass, "#000066 Enter Password: #");

  lv_obj_align(label_pass, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_font(label_pass,&lv_font_montserrat_18,0);

  // Add a text area for password input
  textarea = lv_textarea_create(passwordScreen);
  lv_obj_set_size(textarea, 200, 40);
  lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 45);
  lv_textarea_set_password_mode(textarea, true);
  lv_textarea_set_one_line(textarea, true);

  // Create a custom button matrix
  static const char *btn_map[] = {
      "1", "2", "3","4","5", "\n",
      "6","7", "8", "9","0", "\n",
      "OK", "Cancel",LV_SYMBOL_BACKSPACE, ""
  };

  lv_obj_t *btn_matrix = lv_btnmatrix_create(passwordScreen);
  lv_btnmatrix_set_map(btn_matrix, btn_map);
  lv_obj_set_size(btn_matrix, 250, 150); // Adjust size as needed
  lv_obj_align(btn_matrix, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Event handler for the button matrix
  lv_obj_add_event_cb(btn_matrix, passClickEvent , LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btnm, mainSelectClickEvent, LV_EVENT_CLICKED, NULL);
}

void tabviewEventCb(lv_event_t* e) {
  uint16_t newTabId = lv_tabview_get_tab_act(tabview);  
  if (newTabId == 2) {
    currIsSetup = true;
  }
  else{
    if (currIsSetup) {
      std::vector<int> newOnSensors = findNewOnSensor();
      std::vector<int> newOffSensors = findNewOffSensor();
      if ((newOnSensors.size() != 0) || (newOffSensors.size() != 0)) {
        lv_tabview_set_act(tabview,2,LV_ANIM_ON);
        lv_obj_t* currMsgBox = lv_msgbox_create(
            NULL,
            "Changes were not saved",
            "Please choose Save/Discard",
            NULL,
            true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
      }
      else{
        currIsSetup = false;
      } 
    }
  }
}

void setupMainUI() {
    mainUIScreen = lv_obj_create(NULL); // NULL creates a new screen
    tabview = lv_tabview_create(mainUIScreen, LV_DIR_TOP, 30);
    lv_obj_set_style_bg_color(tabview,HEX_LIGHT_GRAY,0);

    // Add tabs
    homeTab = lv_tabview_add_tab(tabview, "Home");
    statTab = lv_tabview_add_tab(tabview, "Stat");
    setupTab = lv_tabview_add_tab(tabview, "Setup");

    if (!isUser) {
      techTab = lv_tabview_add_tab(tabview, "Tech");
      lv_obj_set_style_bg_color(techTab, HEX_MEDIUM_GRAY, 0);
      lv_obj_set_style_bg_opa(techTab, LV_OPA_COVER, 0);
    }
    if (!isUser && !isTech){
        debugTab = lv_tabview_add_tab(tabview, "Debug");
    }

    createControlsForMain(homeTab);
    createControlsForStat(statTab);
    createControlsForSetup(setupTab);
    if (!isUser) {
      createControlsForTech(techTab);
    }
    if (!isUser && !isTech){
      createDebugModeControls(debugTab);
    }

    lv_obj_add_event_cb(tabview, tabviewEventCb, LV_EVENT_VALUE_CHANGED, NULL);
}

void returnToMain(lv_event_t *e) {    
    // Get the object that triggered the event
    lv_obj_t *btn = lv_event_get_target(e);    

    // Check if the event is a "clicked" event
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
      isTech = false;
      isUser = false;
      
      for (auto& obj_td: objsToDeleteSensors) {
          lv_obj_del(obj_td);
      }
      for (auto& obj_td: objsToDeleteMotors) {
          lv_obj_del(obj_td);
      }
      objsToDeleteSensors.clear();
      objsToDeleteMotors.clear();
      currentEditSensorId = -1;
      currentEditMotorId = -1;

      currentEditSensorSlidersVec.clear();
      currentEditMotorSlidersVec.clear();
      if (debugTab) {
        deleteDebug();
      }
      lv_scr_load_anim(initialUserScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

static void yamlPollTimerCb(lv_timer_t* timer) {
    // If BLE dropped while waiting for YAML, clean up and return to search screen
    if (!hasClient.test_and_set()) {
        hasClient.clear();
        lv_timer_del(timer);
        s_yamlPollTimer = nullptr;
        if (sensorsYamlBuffer) { free(sensorsYamlBuffer); sensorsYamlBuffer = nullptr; }
        if (motorsYamlBuffer) { free(motorsYamlBuffer); motorsYamlBuffer = nullptr; }
        if (funcsYamlBuffer) { free(funcsYamlBuffer); funcsYamlBuffer = nullptr; }
        if (generalYamlBuffer) { free(generalYamlBuffer); generalYamlBuffer = nullptr; }
        return;
    }
    // Parse each YAML section as it becomes ready
    if (isYmlSensorsReady) {
        sensors.clear();
        splitSensorsField((char*)*pointerToSensorBuff);
        isYmlSensorsReady = false;
        free(*pointerToSensorBuff);
        *pointerToSensorBuff = nullptr;
    }
    if (isYmlMotorsReady) {
        motors.clear();
        splitMotorsField((char*)*pointerToMotorsBuff);
        isYmlMotorsReady = false;
        free(*pointerToMotorsBuff);
        *pointerToMotorsBuff = nullptr;
    }
    if (isYmlFunctionsReady) {
        functions.clear();
        splitFunctionsField((char*)*pointerToFuncBuff);
        isYmlFunctionsReady = false;
        free(*pointerToFuncBuff);
        *pointerToFuncBuff = nullptr;
    }
    if (isYmlGeneralReady) {
        generalEntries.clear();
        splitGeneralField((char*)*pointerToGeneralBuff);
        isYmlGeneralReady = false;
        free(*pointerToGeneralBuff);
        *pointerToGeneralBuff = nullptr;
        yamlStructsReady = true;
    }
    // All YAML sections received — finalize and move to main UI
    if (yamlStructsReady) {
        yamlStructsReady = false;
        lv_timer_del(timer);
        s_yamlPollTimer = nullptr;
        xTaskCreatePinnedToCore(BLENotifyTask, "BLE Notify Task", 2048, NULL, 2, &bleNotifyTaskHandle, 0);
        pinMode(buttonPin, INPUT_PULLUP);
        attachInterrupt(buttonPin, buttonPress, RISING);
        setupInitialUserScreen();
    }
}

void loadYamlStep(lv_event_t* e) {
  if (hasClient.test_and_set()) {
      if (sendYamlRequest) {
        SendNotifyToClient("Please send YAML data", YAML_REQ, pCharacteristic);
        Serial.println("Sent yaml request");
        sendYamlRequest = false;
      }
      // YAML still arriving — start non-blocking timer instead of blocking/recursing
      if (s_yamlPollTimer == nullptr) {
        s_yamlPollTimer = lv_timer_create(yamlPollTimerCb, 100, NULL);
      }
  }
  else {
    hasClient.clear();
    lv_scr_load_anim(searchBleScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
  }
}