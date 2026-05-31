#include <lvgl.h>
#include <atomic>
#include <Arduino_GFX_Library.h>
#include <vector>
#include <esp32-hal.h>
#include <NimBLEDevice.h>
#include <string>
#include <Arduino.h>
#include "ConfigParams.h"
#include "Touch.h"
#include "BLEServer.h"

char selectedTextToTitle[32]; 

const int buttonPin = 0; // PIN NUMBER OF EMERGEANCY BUTTON
volatile unsigned long lastPressTime = 0;  // Stores the last press time
const unsigned long debounceDelay = 1000;  // Min time between presses (ms). used to avoid stack overflow

void IRAM_ATTR buttonPress() {
      unsigned long currentTime = millis();
    if (currentTime - lastPressTime < debounceDelay) {
        return;  // Ignore if pressed too quickly
    }
    lastPressTime = currentTime;

    if (bleNotifyTaskHandle == NULL) {
        ets_printf("❌ ERROR: Notify Task Handle is NULL in ISR!\n");
        return;
    }

    Serial.printf("ISR triggered!\n");
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(bleNotifyTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

struct ReturnUnsavedParam{
  int sensorId;
  std::vector<int> paramsId;
  std::vector<int> newVals;
};

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

lv_obj_t* createNewBtn(
    lv_obj_t* parent,
    lv_coord_t w,
    lv_coord_t h,
    lv_align_t align,
    lv_coord_t xOfs,
    lv_coord_t yOfs,
    const char* name,
    lv_color_t btnColor,
    lv_color_t labelColor,
    const lv_font_t* labelSize = &lv_font_montserrat_14)
{
    lv_obj_t *new_btn = lv_btn_create(parent); // Create button with homeTab as parent
    lv_obj_set_size(new_btn, w, h); // Set button size

    // Set the button color
    lv_obj_set_style_bg_color(new_btn, btnColor, 0);


    lv_obj_align(new_btn, align, xOfs, yOfs); // Align to bottom left
    lv_obj_t *label = lv_label_create(new_btn);
    lv_obj_set_style_text_color(label, labelColor, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_AUTO, 0);
    lv_obj_set_style_text_font(label, labelSize, 0);


    lv_label_set_text(label, name);
    return new_btn;
} 

void designBtnm(lv_event_t * e) {
  lv_color_t* color = (lv_color_t*) lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS){
    dsc->rect_dsc->bg_color = *color;
  }
}

void designLabelBig(lv_event_t * e) {
  lv_color_t* color = (lv_color_t*) lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS){
    dsc->label_dsc->color = *color;
    dsc->label_dsc->font = &lv_font_montserrat_20;
  }
}

void designLabelSmall(lv_event_t * e) {
  lv_color_t* color = (lv_color_t*) lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS){
    dsc->label_dsc->color = *color;
    dsc->label_dsc->font = &lv_font_montserrat_10;
  }
}

void gesturesClickEvent(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t * labelCurrentText = lv_obj_get_child(btn, 0);
    char* currentText = lv_label_get_text(labelCurrentText);


    if(code == LV_EVENT_PRESSED ){
      if (!(canPlayGesture.test_and_set())) {
        canPlayGesture.clear();
        lv_label_set_text(label, "#c30a12  Can't Play#\n #c30a12 gesture #");
      }
      else{
        canPlayGesture.clear();

        sendingGesture(currentText); 
        char newText[40]; // Adjust size as needed
        snprintf(newText, sizeof(newText), "#047a04  Playing:\n %s#", currentText);
        lv_label_set_text(label, newText);
      }
    }
    else if(code == LV_EVENT_CLICKED){
      lv_label_set_text(label, " ");
    }

}

lv_obj_t* createNewMatrixBtnChooseOne(
    lv_obj_t * parent,
    char *** map,
    int lenMap,
    lv_align_t align,
    lv_coord_t xOfs,
    lv_coord_t yOfs,
    lv_color_t bgColor,
    lv_color_t btnsColor,
    lv_color_t labelsColor,
    bool big = true,
    lv_coord_t screenWidth = 320,
    lv_coord_t screenHeight = 240,
    int maxInRow = -1,
    bool needToFreeOldMap = false)
{
    
    lv_obj_t* btnm = lv_btnmatrix_create(parent);

    // fit labels to matrix
    if (maxInRow > 0) {
      // create modify map
      int newNumPointer = lenMap + (lenMap - 1) / maxInRow;
      char** tempPtr = (char**)malloc(newNumPointer * sizeof(char*));
      int j = 0;
      for (int i = 0; i < lenMap - 1; i++) {
        tempPtr[j] = *((*map) + i);
        if(i % maxInRow == (maxInRow - 1)){
          j++;
          tempPtr[j] = "\n";
        }
        j++;
      }
      tempPtr[j] = "";

      if (needToFreeOldMap) {
        free(*map);
      }
      (*map) = tempPtr;
    }

    lv_btnmatrix_set_map(btnm, (const char**)(*map));
    lv_btnmatrix_set_one_checked(btnm, true); // Only one button can be checked at a time
    lv_obj_set_size(btnm, screenWidth * 0.7, screenHeight * 0.7);
    lv_obj_set_style_bg_color(btnm, bgColor, 0);
    
    // design each btmn color
    static lv_color_t btnsColorStatic = btnsColor;
    lv_obj_add_event_cb(btnm, designBtnm, LV_EVENT_DRAW_PART_BEGIN, &btnsColorStatic);
    static lv_color_t labelsColorStatic = labelsColor;
    if (big) lv_obj_add_event_cb(btnm, designLabelBig, LV_EVENT_DRAW_PART_BEGIN, &labelsColorStatic);
    else lv_obj_add_event_cb(btnm, designLabelSmall, LV_EVENT_DRAW_PART_BEGIN, &labelsColorStatic);
    lv_obj_align(btnm, align, xOfs, yOfs); // Position below the label
    return btnm;
}

int getGestNum() {
    int numGest = 0;
    for (const auto& function : functions) {
        if(function.protocolType == FUNC_TYPE_GESTURE){
          numGest++;
        }
    }
    return numGest;
}

void createControlsForMain(lv_obj_t* parent) {
// Add the "Return" button to home tab
    lv_obj_t* returnBtn = createNewBtn(
        parent,
        80,
        40,
        LV_ALIGN_BOTTOM_MID,
        -110,
        0,
        "Return",
        HEX_RED,
        HEX_WHITE,
        &lv_font_montserrat_16);
    lv_obj_add_event_cb(returnBtn, returnToMain, LV_EVENT_CLICKED, NULL); // Set the event handler
    lv_obj_set_style_text_align(returnBtn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t * label_home = lv_label_create(parent);
    lv_label_set_recolor(label_home, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label_home, "#000099 S##000066 P##00007f M##0000b2 T#");
    lv_obj_set_style_text_align(label_home, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_home, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(label_home,&lv_font_montserrat_18,0);

    lv_obj_t * label_home_more = lv_label_create(parent);
    lv_label_set_recolor(label_home_more, true);        
    lv_obj_set_width(label_home_more, 75);
    lv_label_set_text(label_home_more, "#000099 Powered by IOT Lab Technion#");
    lv_obj_set_style_text_align(label_home_more, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(label_home_more, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_style_text_font(label_home_more,&lv_font_montserrat_12,0);

    lv_obj_t * label_home_BLE_1 = lv_label_create(parent);
    lv_obj_t * label_home_BLE_2 = lv_label_create(parent);
    lv_label_set_recolor(label_home_BLE_1, true);  
    lv_label_set_recolor(label_home_BLE_2, true);        
      
    lv_obj_set_width(label_home_BLE_1, 75);
    lv_obj_set_width(label_home_BLE_2, 55);

    lv_label_set_text(label_home_BLE_1,  LV_SYMBOL_BLUETOOTH );

    if (hasClient.test_and_set()) {
        lv_label_set_text(label_home_BLE_2,  LV_SYMBOL_OK);
        lv_obj_set_style_text_color(label_home_BLE_1, HEX_ROYAL_BLUE, 0); // Green for ON
        lv_obj_set_style_text_color(label_home_BLE_2, HEX_GREEN, 0); // Green for ON
    }
    else{
        lv_label_set_text(label_home_BLE_2,  LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(label_home_BLE_2, HEX_RED, 0); // Red for OFF
        hasClient.clear();
    }
    
    lv_obj_set_style_text_align(label_home_BLE_1, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(label_home_BLE_2, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(label_home_BLE_1, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_obj_align(label_home_BLE_2, LV_ALIGN_TOP_LEFT, 22, 80);
    lv_obj_set_style_text_font(label_home_BLE_1,&lv_font_montserrat_22,0);
    lv_obj_set_style_text_font(label_home_BLE_2,&lv_font_montserrat_22,0);

    int numGest = getGestNum();
    int numFunctionTotal = functions.size();

    lv_obj_t** gesturesMatrix = (lv_obj_t**)malloc((numGest) * sizeof(lv_obj_t*));
    int maxInRow = 2;


    int j = 0;
    for (int i = 0; i < numFunctionTotal; i++) {
      if(functions[i].protocolType == FUNC_TYPE_GESTURE){
        const char* temp_str = (functions[i].name).c_str();
        gesturesMatrix[j] = createNewBtn(
            parent,
            90,
            30,
            LV_ALIGN_TOP_RIGHT,
            -7 - (j % maxInRow) * 95,
            20 + (j / maxInRow) * 35,
            temp_str,
            HEX_DARK_BLUE,
            HEX_WHITE);
        j++;
      }
    }

    lv_obj_t * infoLabel = lv_label_create(parent);
    // int labelOfffsetY = std::max( 30 + ((j-1)/maxInRow)*35, 270);
    lv_obj_align(infoLabel,LV_ALIGN_BOTTOM_LEFT, 0, -45);
    lv_obj_set_width(infoLabel, 84);
    lv_label_set_recolor(infoLabel, true);
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_12, 0);
    lv_label_set_text(infoLabel, " ");

    for (int k = 0; k < j; k++) {
        lv_obj_add_event_cb(gesturesMatrix[k], gesturesClickEvent, LV_EVENT_PRESSED, infoLabel);
        lv_obj_add_event_cb(gesturesMatrix[k], [](lv_event_t* e) { delay(800);}, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(gesturesMatrix[k], gesturesClickEvent, LV_EVENT_CLICKED, infoLabel);
    }



        // gestures matrix hadder
    lv_obj_t* labelGest = lv_label_create(parent);
    lv_label_set_text(labelGest, "Gestures");
    // lv_obj_center(label);
    lv_obj_align(labelGest,LV_ALIGN_TOP_RIGHT, -64, -5);
    lv_obj_set_style_text_font(labelGest,&lv_font_montserrat_18,0);
    // lv_obj_add_event_cb(return_btn, returnToMain, LV_EVENT_CLICKED, NULL); // Set the event handler
}

std::vector<int> findNewOffSensor() {
  std::vector<int> retVec;
  for (int i = 0; i < sensors.size(); i++) {
    if((sensors[i].status.equalsIgnoreCase("ON")) && (!lv_obj_has_state(sensorSwitchVec[i],LV_STATE_CHECKED))){
      retVec.push_back(i);
    }
  }
  return retVec;
}

std::vector<int> findNewOnSensor() {
  std::vector<int> retVec;
  for (int i = 0; i < sensors.size(); i++) {
    if((sensors[i].status.equalsIgnoreCase("OFF")) && (lv_obj_has_state(sensorSwitchVec[i],LV_STATE_CHECKED))){
      retVec.push_back(i);
    }
  }
  return retVec;
}

void createControlsForStat(lv_obj_t* parent) {
  // Create a title label
  lv_obj_t* titleLabel = lv_label_create(parent); 
  lv_label_set_text(titleLabel, "Sensors State:"); 
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10); 
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_18, 0); 

  int yOffset = 40; // Vertical spacing between rows

  // Loop through each sensor
  for (const auto& sensor : sensors) { 
      // Replace "_" in the name with spaces
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);



      // Set the style of the status label based on the sensor status
      if (sensor.status.equalsIgnoreCase("ON")) {
          // Create a label for the sensor name
          lv_obj_t* sensorNameLabel = lv_label_create(parent); 
          lv_label_set_text(sensorNameLabel, (display_name + ":").c_str()); 
          lv_obj_align(sensorNameLabel, LV_ALIGN_TOP_LEFT, 15, yOffset); 
          lv_obj_set_style_text_font(sensorNameLabel, &lv_font_montserrat_16, 0); 

          // Create a status label
          lv_obj_t* sensorStatusLabel = lv_label_create(parent); 
          lv_obj_align(sensorStatusLabel, LV_ALIGN_TOP_RIGHT, -15, yOffset); 
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);
          
          lv_label_set_text(sensorStatusLabel, "ON " LV_SYMBOL_OK); 
          lv_obj_set_style_text_color(sensorStatusLabel, HEX_GREEN, 0); // Green for ON
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);  // Slightly larger font
          yOffset += 28;
      } 

      // Update the vertical offset for the next sensor
      
  }
  for (const auto& sensor : sensors){
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);

    if (sensor.status.equalsIgnoreCase("OFF")) {
          // Create a label for the sensor name
          lv_obj_t* sensor_name_label = lv_label_create(parent); 
          lv_label_set_text(sensor_name_label, (display_name + ":").c_str()); 
          lv_obj_align(sensor_name_label, LV_ALIGN_TOP_LEFT, 15, yOffset); 
          lv_obj_set_style_text_font(sensor_name_label, &lv_font_montserrat_16, 0); 

          // Create a status label
          lv_obj_t* sensorStatusLabel = lv_label_create(parent); 
          lv_obj_align(sensorStatusLabel, LV_ALIGN_TOP_RIGHT, -15, yOffset); 
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);
      
          lv_label_set_text(sensorStatusLabel, "OFF " LV_SYMBOL_CLOSE); 
          lv_obj_set_style_text_color(sensorStatusLabel,HEX_RED, 0); // Red for OFF
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);  // Slightly larger font

          yOffset += 28;

    }
  }
}

void discardBtnClickEvent(lv_event_t * e) {

  for (int i=0; i< sensors.size(); i++) {
        if(sensors[i].status.equalsIgnoreCase("ON")){
          lv_obj_add_state(sensorSwitchVec[i], LV_STATE_CHECKED);
        }
        else if(sensors[i].status.equalsIgnoreCase("OFF")){
          lv_obj_clear_state(sensorSwitchVec[i], LV_STATE_CHECKED);
        }
        else{
          Serial.printf("ERROR PLEASE CHECK");
        }
  }
  
}

void discardSensorBtnClickEvent(lv_event_t * e) {
  struct ReturnUnsavedParam unsaveStruct = checkUnsaveSensorParam();
  if (unsaveStruct.paramsId.size() > 0) {
    // int param_slider_id = 0;
    int i = 0;
    for (const auto& [paramName, param] : sensors[currentEditSensorId].function.parameters){
      if (param.currentVal != lv_slider_get_value(currentEditSensorSlidersVec[i])) {
        lv_slider_set_value(currentEditSensorSlidersVec[i], param.currentVal, LV_ANIM_ON);
        lv_event_send(currentEditSensorSlidersVec[i], LV_EVENT_VALUE_CHANGED, NULL);
      }
      i++;
    }
    

  }

}

void discardMotorBtnClickEvent(lv_event_t * e) {
  struct ReturnUnsavedParam unsaveTsh = checkUnsaveMotorParam();
  if (unsaveTsh.paramsId.size() > 0) {
    lv_slider_set_value(
        currentEditMotorSlidersVec[0],
        motors[currentEditMotorId].safetyThreshold.currentVal,
        LV_ANIM_ON);
    lv_event_send(currentEditMotorSlidersVec[0], LV_EVENT_VALUE_CHANGED, NULL);
  }
}

void saveNewSwitchBtnmToStruct() {
  for (int i=0; i< sensors.size(); i++) {
    if(!lv_obj_has_state(sensorSwitchVec[i],LV_STATE_CHECKED)){
      sensors[i].status = "off";
    }
    else if(lv_obj_has_state(sensorSwitchVec[i],LV_STATE_CHECKED)){
      sensors[i].status = "on";
    }
    else{
      Serial.printf("PROBLEM IN SAVE NEW BTM");
    }
  }
}

void saveNewMotorValToStruct(struct ReturnUnsavedParam motorThsStruct) {
    motors[motorThsStruct.sensorId].safetyThreshold.currentVal = motorThsStruct.newVals[0];
}

void saveNewSensorsValToStruct(struct ReturnUnsavedParam sensorsStruct) {
    int i = 0;
    int j = 0;
    for(auto& [name, param] : sensors[sensorsStruct.sensorId].function.parameters){
      if(sensorsStruct.paramsId[i] == j){
        param.currentVal = sensorsStruct.newVals[i];
        i++;
      }
      j++;
    }
}

void saveBtnApproved(lv_event_t * e) {
  if (sendNewSwitchesMsgBox) {
    lv_msgbox_close(sendNewSwitchesMsgBox);
    sendNewSwitchesMsgBox = NULL;
  }
  saveNewSwitchBtnmToStruct();
  lv_obj_clean(statTab);
  createControlsForStat(statTab);
  lv_obj_t* currMsgBox =
      lv_msgbox_create(NULL, LV_SYMBOL_OK, "Changes have been saved.\nProthesis is updated.", NULL, true);
  lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
}

void saveBtnTechMotorApproved(lv_event_t * e) {
  if (sendNewSwitchesMsgBox) {
    lv_msgbox_close(sendNewSwitchesMsgBox);
    sendNewSwitchesMsgBox = NULL;
  }
  struct ReturnUnsavedParam unsaveMotorThs = checkUnsaveMotorParam();
  saveNewMotorValToStruct(unsaveMotorThs);
  lv_obj_t* currMsgBox =
      lv_msgbox_create(NULL, LV_SYMBOL_OK, "Changes have been saved.\nProthesis is updated.", NULL, true);
  lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
}

void saveBtnTechSensorApproved(lv_event_t * e) {
  if (sendNewSwitchesMsgBox) {
    lv_msgbox_close(sendNewSwitchesMsgBox);
    sendNewSwitchesMsgBox = NULL;
  }
  struct ReturnUnsavedParam unsaveSensors = checkUnsaveSensorParam();
  saveNewSensorsValToStruct(unsaveSensors);

  lv_obj_t* currMsgBox =
      lv_msgbox_create(NULL, LV_SYMBOL_OK, "Changes have been saved.\nProthesis is updated.", NULL, true);
  lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
}

void saveBtnClickEvent(lv_event_t * e) {
    std::vector<int> newOnSensors = findNewOnSensor();
    std::vector<int> newOffSensors = findNewOffSensor();
    if ((newOnSensors.size() == 0) && (newOffSensors.size() == 0)) {
      lv_obj_t* currMsgBox =lv_msgbox_create(NULL,"Nothing to save...",NULL,NULL, true);
      lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
    }
    else{
      if (isDemoYaml.test_and_set()) {
        saveNewSwitchBtnmToStruct();
        lv_obj_clean(statTab);
        createControlsForStat(statTab);
        lv_obj_t* currMsgBox =lv_msgbox_create(NULL,LV_SYMBOL_OK,"Changes have been saved.",NULL, true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
      }
      else{
        isDemoYaml.clear();
        notFinishUpdateSensors.test_and_set();
        sendNewSwitchesMsgBox = lv_msgbox_create(
            NULL,
            "Saving changes...",
            "Sending new sensors states to prosthesis.",
            NULL,
            false);
        lv_obj_center(sendNewSwitchesMsgBox);
        lv_refr_now(NULL);
        SendStatusChangeReq(newOnSensors, newOffSensors);  
        delay(1000);
        lv_refr_now(NULL);
        while (notFinishUpdateSensors.test_and_set()) {
          delay(200);
          if (!notRemoveBox.test_and_set()) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
            break;
          }
        }
        if ((!notRemoveBox.test_and_set()) || (!notFinishUpdateSensors.test_and_set())) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
        }
        else{
          lv_event_send(saveBtn, EVENT_SENSOR_CHANGED_SECC, NULL);  
        }
      }
    }
}

void saveBtnTechMotorClickEvent(lv_event_t * e) {
    struct ReturnUnsavedParam unsaveMotorThs = checkUnsaveMotorParam();

    if (unsaveMotorThs.paramsId.size() == 0) {

      lv_obj_t* currMsgBox =lv_msgbox_create(NULL,"Nothing to save...",NULL,NULL, true);
      lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
    }
    else{
      if (isDemoYaml.test_and_set()) {
        saveNewMotorValToStruct(unsaveMotorThs);
        lv_obj_t* currMsgBox =lv_msgbox_create(NULL,LV_SYMBOL_OK,"Changes have been saved.",NULL, true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
      }
      else{
        isDemoYaml.clear();
        notFinishUpdateSensors.test_and_set();
        sendNewSwitchesMsgBox = lv_msgbox_create(
            NULL,
            "Saving changes...",
            "Sending new motor's safety threshold to prosthesis.",
            NULL,
            false);
        lv_obj_center(sendNewSwitchesMsgBox);
        lv_refr_now(NULL);

        SendMotorParamChangeReq(unsaveMotorThs.sensorId, unsaveMotorThs.newVals);

        delay(1000);
        lv_refr_now(NULL);
        while (notFinishUpdateSensors.test_and_set()) {
          delay(200);
          if (!notRemoveBox.test_and_set()) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
            break;
          }
        }
        if ((!notRemoveBox.test_and_set()) || (!notFinishUpdateSensors.test_and_set())) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
        }
        else{
          lv_event_send(saveBtnTechMotors, EVENT_SENSOR_CHANGED_SECC, NULL);  
        }
      }
    }
}

void saveBtnTechSensorClickEvent(lv_event_t * e) {
    struct ReturnUnsavedParam unsaveSensors = checkUnsaveSensorParam();

    if (unsaveSensors.paramsId.size() == 0) {
      lv_obj_t* currMsgBox =lv_msgbox_create(NULL,"Nothing to save...",NULL,NULL, true);
      lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
    }
    else{
      if (isDemoYaml.test_and_set()) {
        saveNewSensorsValToStruct(unsaveSensors);
        lv_obj_t* currMsgBox =lv_msgbox_create(NULL,LV_SYMBOL_OK,"Changes have been saved.",NULL, true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
      }
      else{
        isDemoYaml.clear();
        notFinishUpdateSensors.test_and_set();
        sendNewSwitchesMsgBox = lv_msgbox_create(
            NULL,
            "Saving changes...",
            "Sending new sensor's parameters to prosthesis.",
            NULL,
            false);
        lv_obj_center(sendNewSwitchesMsgBox);
        
        lv_refr_now(NULL);

        SendSensorParamChangeReq(
            unsaveSensors.sensorId,
            unsaveSensors.paramsId,
            unsaveSensors.newVals);

        delay(1000);
        lv_refr_now(NULL);
        while (notFinishUpdateSensors.test_and_set()) {
          delay(200);
          if (!notRemoveBox.test_and_set()) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
            break;
          }
        }
        if ((!notRemoveBox.test_and_set()) || (!notFinishUpdateSensors.test_and_set())) {
            if (sendNewSwitchesMsgBox) {
              lv_msgbox_close(sendNewSwitchesMsgBox);
              sendNewSwitchesMsgBox = NULL;
            }
        }
        else{
          lv_event_send(saveBtnTechSensors, EVENT_SENSOR_CHANGED_SECC, NULL);  
        }
      }
    }
}

void createControlsForSetup(lv_obj_t* parent) {
  // Create a title label
  lv_obj_t* titleLabel = lv_label_create(parent); 
  lv_label_set_text(titleLabel, "Sensors Setup:"); 
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10); 
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_18, 0); 

  int yOffset = 40; // Vertical spacing between rows

  static lv_style_t style_indic_on;
  static lv_style_t style_indic_off;
  static lv_style_t style_knob_on;
  static lv_style_t style_knob_off;

  lv_style_init(&style_indic_on);
  lv_style_set_bg_color(&style_indic_on, HEX_DARK_BLUE);
  lv_style_set_bg_opa(&style_indic_on, LV_OPA_COVER);

  lv_style_init(&style_indic_off);
  lv_style_set_bg_color(&style_indic_off,  HEX_LIGHT_GRAY_2); 
  lv_style_set_bg_opa(&style_indic_off, LV_OPA_COVER);

  // Loop through each sensor
  for (const auto& sensor : sensors) { 
      // Replace "_" in the name with spaces
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);
      
      // Create a label for the sensor name
      lv_obj_t* sensor_name_label = lv_label_create(parent); 
      lv_label_set_text(sensor_name_label, (display_name + ":").c_str()); 
      lv_obj_align(sensor_name_label, LV_ALIGN_TOP_LEFT, 15, yOffset+5); 
      lv_obj_set_style_text_font(sensor_name_label, &lv_font_montserrat_16, 0); 

      // Create a status label
      lv_obj_t* sensorSwitch = lv_switch_create(parent); 
      sensorSwitchVec.push_back(sensorSwitch);
     
      lv_obj_align(sensorSwitch, LV_ALIGN_TOP_RIGHT, -15, yOffset); 

      lv_obj_add_style(sensorSwitch, &style_indic_on, LV_PART_INDICATOR | LV_STATE_CHECKED);
      lv_obj_add_style(sensorSwitch, &style_indic_off, LV_PART_INDICATOR | LV_STATE_DEFAULT);

      if(sensor.status.equalsIgnoreCase("ON")){
        lv_obj_clear_state(sensorSwitch, LV_STATE_DEFAULT);
        lv_obj_add_state(sensorSwitch, LV_STATE_CHECKED);
      }
      else if(sensor.status.equalsIgnoreCase("OFF")){
        lv_obj_clear_state(sensorSwitch, LV_STATE_CHECKED);
        lv_obj_add_state(sensorSwitch, LV_STATE_DEFAULT);
      }
      else{
        Serial.printf("ERROR PLEASE CHECK");
      }
      yOffset += 37;  
  }
  saveBtn = createNewBtn(
      parent,
      90,
      40,
      LV_ALIGN_TOP_MID,
      -50,
      yOffset + 8,
      "Save",
      HEX_GREEN,
      HEX_WHITE,
      &lv_font_montserrat_16);
  lv_obj_t* discardBtn = createNewBtn(
      parent,
      90,
      40,
      LV_ALIGN_TOP_MID,
      50,
      yOffset + 8,
      "Discard",
      HEX_RED,
      HEX_WHITE,
      &lv_font_montserrat_16);
  lv_obj_add_event_cb(saveBtn, saveBtnClickEvent, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(saveBtn, saveBtnApproved, EVENT_SENSOR_CHANGED_SECC, NULL);
  lv_obj_add_event_cb(discardBtn, discardBtnClickEvent, LV_EVENT_CLICKED, NULL);
}

// Function to generate dummy sensor data (Replace with real sensor function later)
static int getSensorValue() {
    return lv_rand(10, 90); // Simulated sensor value between 10 and 90
}

static void updateChartReq(lv_timer_t *t) {
  int* arr = static_cast<int*>(t->user_data);
  char* msgToSend=(char*)malloc(MAX_MSG_LEN);
  int ind = 0;
  String isMotorStr =String(arr[0]);
  strcpy(&(msgToSend[ind]), isMotorStr.c_str());
  ind+=isMotorStr.length();
  strcpy(&msgToSend[ind++],"|");
  String hardwareIdStr =String(arr[1]);
  strcpy(&(msgToSend[ind]), hardwareIdStr.c_str());
  SendNotifyToClient(msgToSend, READ_REQ, pCharacteristic);
  free(msgToSend);
}

// Timer callback to update the chart
static void updateChart(lv_timer_t *t) {
    lv_chart_set_next_value(chart, ser, getSensorValue());

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

// Button event callback to show the chart
static void showChartEventCB(bool isMotor, int id) {

    // Hide elements that we dont want right now
    lv_obj_add_flag(dropdownMotorsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dropdownSensorsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(TabviewObjDebugMode, LV_OBJ_FLAG_HIDDEN);

    // Create the chart
    chart = lv_chart_create(debugTab);

    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_size(chart, 280, 125);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -30);

    // Set chart parameters
    lv_chart_set_point_count(chart, numPoints);
    ser = lv_chart_add_series(chart, HEX_DARK_BLUE, LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);    
    static int arr[2];  // Declare static array without initialization
    arr[0] = (int)isMotor;  // Update dynamically
    arr[1] = id;

    if (isDemoYaml.test_and_set()) {
      if (chartTimer) {
        lv_timer_del(chartTimer); // Stop the timer
        chartTimer = NULL;
      }
      chartTimer = lv_timer_create(updateChart, 200, static_cast<void*>(arr)); // updateChart without BLE

    }else{
      isDemoYaml.clear();
      chartTimer = lv_timer_create(updateChartReq, 200, static_cast<void*>(arr)); // updateChartReq - request
    }
    // Create the "Close Chart" button
    closeChartBTN = lv_btn_create(debugTab);
    lv_obj_set_size(closeChartBTN, 100, 25);
    lv_obj_align(closeChartBTN, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_set_style_bg_color(closeChartBTN, HEX_DARK_BLUE, 0);
    lv_obj_t * label = lv_label_create(closeChartBTN);
    lv_label_set_text(label, "Close Chart");
    lv_obj_center(label);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_12,0);

    lv_obj_add_event_cb(closeChartBTN, closeChartEventCB, LV_EVENT_CLICKED, NULL);
    // Create a title label
    debugLabel = lv_label_create(debugTab); 
    lv_label_set_text(debugLabel, selectedTextToTitle);
    lv_obj_align(debugLabel, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_set_style_text_font(debugLabel, &lv_font_montserrat_18, 0);
}
// Button event callback to close the chart
static void closeChartEventCB(lv_event_t * e) {
  if (chartTimer) {
    lv_timer_del(chartTimer); // Stop the timer
    chartTimer = NULL;
  }
  
  delay(200); // To make sure that the last message was handled properly

  if (chart) {// Delete the chart
    lv_obj_del(chart);
    chart = NULL;
  }
  if (closeChartBTN) {// Delete the close button
    lv_obj_del(closeChartBTN);
    closeChartBTN = NULL;
  }
  ser = NULL;
  if(debugLabel){
    lv_obj_del(debugLabel);
    debugLabel = NULL;
  }
  // Show back the elements  we hid
  lv_obj_clear_flag(dropdownMotorsObj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(dropdownSensorsObj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(TabviewObjDebugMode, LV_OBJ_FLAG_HIDDEN);
}

static void showDropdownChartCB(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    char selected_text[32]; // Buffer for selected item
    lv_dropdown_get_selected_str(dropdown, selected_text, sizeof(selected_text));
    strcpy(selectedTextToTitle,selected_text);
    
    String selectedTextStr = String(selected_text);

    // Determine if this is the motors dropdown or sensors dropdown
    bool isMotor = (dropdown == dropdownMotorsObj);

    int id = 0;

    if (isMotor) {
        for (const auto& motor : motors) { 
            if (motor.name == selectedTextStr) {
                break;
            }
            id++;
        }
        if (id == motors.size()){
          return;
        }
    } else {
        for (const auto& sensor : sensors) {
            if (sensor.name == selectedTextStr) {
                break;
            }
            id++;
        }
        if (id == sensors.size()){
          return;
        }
    }    
    showChartEventCB(isMotor, id);
}

char* getOptionsString(bool isMotors){
  int totalLength  = 0;
  if(isMotors){
    for (const auto& motor : motors) {
      totalLength += motor.name.length() + 1;

    }
    if (totalLength == 0) return nullptr; // No motors, return nullptr
    // Allocate memory for the total length + 1 for the null terminator
    char* result = (char*) malloc(totalLength + 1);
    if (result == nullptr) return nullptr; // Failed to allocate memory

    // Concatenate all names into the allocated space
    char* currentPos = result;
    for (const auto& motor : motors) {
        strcpy(currentPos, motor.name.c_str()); // Copy the motor name
        currentPos += motor.name.length();      // Move the pointer
        *currentPos = '\n';                     // Add a newline character
        currentPos++;                           // Move past the newline
    }
    *(currentPos - 1) = '\0'; // Replace the last newline with a null terminator
    return result;
    
  }
  else{
    for (const auto& sensor : sensors) {
      totalLength += sensor.name.length() + 1;

    }
    if (totalLength == 0) return nullptr; // No motors, return nullptr
    // Allocate memory for the total length + 1 for the null terminator
    char* result = (char*) malloc(totalLength + 1);
    if (result == nullptr) return nullptr; // Failed to allocate memory

    // Concatenate all names into the allocated space
    char* currentPos = result;
    for (const auto& sensor : sensors) {
        strcpy(currentPos, sensor.name.c_str()); // Copy the motor name
        currentPos += sensor.name.length();      // Move the pointer
        *currentPos = '\n';                     // Add a newline character
        currentPos++;                           // Move past the newline
    }
    *(currentPos - 1) = '\0'; // Replace the last newline with a null terminator
        
    return result;
  }
}

lv_obj_t* createDropdownsDebugMode(lv_obj_t* parent, bool isMotors){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char* options = getOptionsString(isMotors);
  if(isMotors){
    lv_dropdown_set_text(dropdown, "Motor");
  } else{
    lv_dropdown_set_text(dropdown, "Sensor");
  }
  lv_dropdown_set_options(dropdown,options);
  lv_dropdown_set_selected_highlight(dropdown, true);
  lv_obj_add_event_cb(dropdown, showDropdownChartCB, LV_EVENT_VALUE_CHANGED, NULL); // Attach event
  if (options){
    free(options);
  }
  return dropdown;
}

void createDebugModeControls(lv_obj_t* parent) {
  // Tabview
    TabviewObjDebugMode = lv_tabview_create(parent, LV_DIR_RIGHT, 70);
    lv_obj_set_style_bg_color(TabviewObjDebugMode,HEX_MEDIUM_GRAY,0);
    lv_obj_set_style_bg_opa(TabviewObjDebugMode, LV_OPA_COVER, 0);
    
    lv_obj_t* DebugModeBTNS = lv_tabview_get_tab_btns(TabviewObjDebugMode);
    lv_obj_set_style_bg_color(DebugModeBTNS, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(DebugModeBTNS, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(DebugModeBTNS, LV_BORDER_SIDE_LEFT, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(DebugModeBTNS, LV_OPA_COVER, 0);        
    lv_obj_t * debug_tab_sensors = lv_tabview_add_tab(TabviewObjDebugMode, "Sensors");
    lv_obj_t * debug_tab_motors = lv_tabview_add_tab(TabviewObjDebugMode, "Motors");

    lv_obj_clear_flag(lv_tabview_get_content(TabviewObjDebugMode), LV_OBJ_FLAG_SCROLLABLE);

    dropdownMotorsObj = createDropdownsDebugMode(debug_tab_motors, true);
    dropdownSensorsObj =createDropdownsDebugMode(debug_tab_sensors, false);
    
}

lv_obj_t* createMotorsTabTech(lv_obj_t* parent){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char* options = getOptionsString(true);
  lv_dropdown_set_text(dropdown, "Motor");
  lv_dropdown_set_options(dropdown,options);
  lv_dropdown_set_selected_highlight(dropdown, true);
  if (options){
    free(options);
  }
  return dropdown;
}

lv_obj_t* createSensorsTabTech(lv_obj_t* parent){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char* options = getOptionsString(false);
  lv_dropdown_set_options(dropdown,options);
  lv_dropdown_set_text(dropdown, "Sensor");
  lv_dropdown_set_selected_highlight(dropdown, true);
  if (options){
    free(options);
  }
  return dropdown;
}

void sliderEventCbAnim(lv_event_t* e) {
  lv_obj_t* valLabel = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t * slider = lv_event_get_target(e);
  lv_label_set_text(valLabel, String(lv_slider_get_value(slider)).c_str());
}

void sliderEventCbUpdatedVal(lv_event_t* e) {
  lv_obj_t * slider = lv_event_get_target(e);
  // Serial.printf("sliders for sensor %d: \n", currentEditSensorId);
  for (int i = 0; i < currentEditSensorSlidersVec.size(); i++) {
    // Serial.printf("%d ",lv_slider_get_value(currentEditSensorSlidersVec[i]));
  }  
  // Serial.printf("\n");
}

void sliderEventCbUpdatedValMotor(lv_event_t* e) {
  lv_obj_t * slider = lv_event_get_target(e);
  // Serial.printf("sliders for motor %d: \n", currentEditMotorId);
  for (int i = 0; i < currentEditMotorSlidersVec.size(); i++) {
    // Serial.printf("%d ",lv_slider_get_value(currentEditMotorSlidersVec[i]));
  }  
  // Serial.printf("\n");
}

struct ReturnUnsavedParam checkUnsaveSensorParam() {
  struct ReturnUnsavedParam retStruct;
  std::vector<int> retParamsId;
  std::vector<int> retNewVals;
  retStruct.sensorId = currentEditSensorId;
  int i = 0;
  if (currentEditSensorSlidersVec.size() > 0) {
    for (const auto& [paramName, param] : sensors[currentEditSensorId].function.parameters) {
      if (param.currentVal != lv_slider_get_value(currentEditSensorSlidersVec[i])) {
        retParamsId.push_back(i);
        retNewVals.push_back(lv_slider_get_value(currentEditSensorSlidersVec[i]));
      }
      i++;
    }
  }
  retStruct.paramsId = retParamsId;
  retStruct.newVals = retNewVals;
  return retStruct;
}

struct ReturnUnsavedParam checkUnsaveMotorParam() {
  struct ReturnUnsavedParam retStruct;
  std::vector<int> retParamsId;
  std::vector<int> retNewVals;
  retStruct.sensorId = currentEditMotorId;
  int i = 0;
  if (currentEditMotorSlidersVec.size() > 0) {
    if (motors[currentEditMotorId].safetyThreshold.currentVal !=
        lv_slider_get_value(currentEditMotorSlidersVec[i])) {
      retParamsId.push_back(i);
      retNewVals.push_back(lv_slider_get_value(currentEditMotorSlidersVec[i]));
    }
    i++;
  }
  retStruct.paramsId = retParamsId;
  retStruct.newVals = retNewVals;
  return retStruct;
}

static void setValue(void * indic, int32_t v)
{
    lv_meter_set_indicator_value(meter, (lv_meter_indicator_t*)indic, v);
}

void motorsActivity(lv_obj_t* parent){
    meter = lv_meter_create(parent);
    lv_obj_align(meter, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_size(meter, 130, 130);
    /*Add a scale first*/
    lv_meter_scale_t * scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);
    lv_meter_indicator_t * indic;
    /*Add a blue arc to the start*/
    indic = lv_meter_add_arc(meter, scale, 3, HEX_DARK_BLUE, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);
    /*Make the tick lines blue at the start of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, HEX_DARK_BLUE, HEX_DARK_BLUE,false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);
    /*Add a red arc to the end*/
    indic = lv_meter_add_arc(meter, scale, 3, HEX_RED, 0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    /*Make the tick lines red at the end of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, HEX_RED, HEX_RED, false,0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    /*Add a needle line indicator*/
    indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);
    /*Create an animation to set the value*/
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, setValue);
    lv_anim_set_var(&a, indic);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, 2000);
    lv_anim_set_repeat_delay(&a, 100);
    lv_anim_set_playback_time(&a, 500);
    lv_anim_set_playback_delay(&a, 100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

void eventViewEditMotors(lv_event_t* e){
  lv_obj_t* parent = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t * obj = lv_event_get_target(e);
  int id = -1;
  if(obj){
    id = lv_dropdown_get_selected(obj);
  }
  Motor& motor = motors[id];
  if (((currentEditMotorId != -1) && (currentEditMotorId != id)) || motorTriggeredByTechTab) {
    if (checkUnsaveMotorParam().paramsId.size() != 0) {
        lv_dropdown_set_selected(obj, currentEditMotorId);
        if (motorTriggeredByTechTab) {
          lv_tabview_set_act(techTabview, 0, LV_ANIM_ON);
        }
        lv_obj_t* currMsgBox = lv_msgbox_create(
            NULL,
            "Changes were not saved",
            "Please choose Save/Discard",
            NULL,
            true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
        motorTriggeredByTechTab = false;
    }
    else {
      if (motorTriggeredByTechTab) {
        motorTriggeredByTechTab = false;
        return;
      }    
      currentEditMotorId = id;
      // delete_all_children_except(parent, obj);
      for (auto& obj_td: objsToDeleteMotors) {
        lv_obj_del(obj_td);
      }
      objsToDeleteMotors.clear();
      currentEditMotorSlidersVec.clear();
      String param_info = "Safety_threshold:\n";
      int yOffset = 45;  // Initial Y position for UI elements
      // Update the label for parameters
      lv_obj_t * label_motor_params = lv_label_create(parent);
      lv_obj_set_width(label_motor_params, 180); // Set width for better display
      lv_obj_align(label_motor_params, LV_ALIGN_TOP_LEFT, -10, yOffset);
      objsToDeleteMotors.push_back(label_motor_params);
      lv_label_set_text(label_motor_params, param_info.c_str());
      yOffset += 25;
      // Iterate over all parameters
      const auto& param = motor.safetyThreshold;
      lv_obj_t* slider = lv_slider_create(parent);
      lv_slider_set_range(slider, param.min, param.max);
      lv_slider_set_value(slider, param.currentVal, LV_ANIM_OFF);
      lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
      lv_obj_set_size(slider, 140, 8);
      lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 10, yOffset);
      objsToDeleteMotors.push_back(slider);
      lv_obj_set_ext_click_area(slider, 10);
      static lv_style_t style_disable;
      lv_style_init(&style_disable);
      lv_style_set_bg_color(&style_disable, HEX_DARK_GRAY);
      lv_style_set_line_color(&style_disable, HEX_DARK_GRAY);
      lv_obj_add_style(slider, &style_disable, LV_PART_INDICATOR | LV_STATE_DISABLED);
      lv_obj_add_style(slider, &style_disable, LV_PART_KNOB | LV_STATE_DISABLED);
      lv_obj_add_style(slider, &style_disable, LV_PART_MAIN | LV_STATE_DISABLED);
      lv_obj_set_style_border_color(slider, HEX_BLACK,LV_PART_INDICATOR);
      lv_obj_t* minValueLabel = lv_label_create(parent);
      lv_label_set_text_fmt(minValueLabel, "%d", param.min);
      lv_obj_align(minValueLabel, LV_ALIGN_TOP_LEFT, -10, yOffset - 5);
      lv_obj_set_style_text_font(minValueLabel, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(minValueLabel);
      lv_obj_t* maxValueLabel = lv_label_create(parent);
      lv_label_set_text_fmt(maxValueLabel, "%d", param.max);
      lv_obj_align(maxValueLabel, LV_ALIGN_TOP_LEFT, 155, yOffset - 5);
      lv_obj_set_style_text_font(maxValueLabel, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(maxValueLabel);
      yOffset += 15;
      // Create a label to display the current value
      lv_obj_t* valueLabel = lv_label_create(parent);
      lv_label_set_text_fmt(valueLabel, "%d", param.currentVal);
      lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
      objsToDeleteMotors.push_back(valueLabel);
      currentEditMotorSlidersVec.push_back(slider);
      lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
      lv_obj_add_event_cb(slider, sliderEventCbUpdatedValMotor, LV_EVENT_RELEASED, NULL);
      yOffset += 20;
      lv_obj_set_user_data(slider, valueLabel);
      // If parameter is not editable, disable the slider
      if (!param.modifyPermission) {
          lv_obj_add_state(slider, LV_STATE_DISABLED);
      }
      saveBtnTechMotors = createNewBtn(
          parent,
          72,
          32,
          LV_ALIGN_TOP_MID,
          -50,
          yOffset + 8,
          "Save",
          HEX_GREEN,
          HEX_WHITE,
          &lv_font_montserrat_14);
      objsToDeleteMotors.push_back(saveBtnTechMotors);
      lv_obj_t* discardBtn = createNewBtn(
          parent,
          72,
          32,
          LV_ALIGN_TOP_MID,
          30,
          yOffset + 8,
          "Discard",
          HEX_RED,
          HEX_WHITE,
          &lv_font_montserrat_14);
      objsToDeleteMotors.push_back(discardBtn);
      yOffset += 50;
      lv_obj_t * labelMotorName = lv_label_create(parent);
      lv_obj_set_width(labelMotorName, 180); // Set width for better display
      lv_obj_align(labelMotorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorName, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(labelMotorName);
      yOffset += 20;
      lv_obj_t * labelMotorType = lv_label_create(parent);
      lv_obj_set_width(labelMotorType, 180); // Set width for better display
      lv_obj_align(labelMotorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorType, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(labelMotorType);
      yOffset += 25;
      lv_label_set_text_fmt(labelMotorName, "Name: %s", motor.name.c_str());
      lv_label_set_text_fmt(labelMotorType, "Type: %s", motor.type.c_str());
      lv_obj_t * labelMotorPin = lv_label_create(parent);
      lv_obj_set_width(labelMotorPin, 180); // Set width for better display
      lv_obj_align(labelMotorPin, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorPin, &lv_font_montserrat_14, 0);
      lv_label_set_text(labelMotorPin, "Pins:");
      objsToDeleteMotors.push_back(labelMotorPin);

      yOffset += 20;
      // Iterate over all parameters
      int i = 0;
      for (const auto& param: motor.pins) {
            lv_obj_t * labelPinType = lv_label_create(parent);
            lv_obj_set_width(labelPinType, 180); // Set width for better display
            lv_obj_align(labelPinType, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(labelPinType, &lv_font_montserrat_12, 0);
            objsToDeleteMotors.push_back(labelPinType);
            lv_label_set_text_fmt(labelPinType, "Pin Type: %s", param.type.c_str());
            yOffset += 20; 

            lv_obj_t * labelPinNumber = lv_label_create(parent);
            lv_obj_set_width(labelPinNumber, 180);
            lv_obj_align(labelPinNumber, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(labelPinNumber, &lv_font_montserrat_12, 0);
            objsToDeleteMotors.push_back(labelPinNumber);
            lv_label_set_text_fmt(labelPinNumber, "Pin Number: %d", param.pinNumber);
            yOffset += 20; 
      }

      lv_obj_add_event_cb(saveBtnTechMotors, saveBtnTechMotorClickEvent, LV_EVENT_CLICKED, NULL);
      lv_obj_add_event_cb(saveBtnTechMotors, saveBtnTechMotorApproved, EVENT_SENSOR_CHANGED_SECC, NULL);
      lv_obj_add_event_cb(discardBtn, discardMotorBtnClickEvent, LV_EVENT_CLICKED, NULL);
      
    }
  }
  else if (currentEditMotorId == id) {
    return;
  }
  else {
    if (motorTriggeredByTechTab) {
      motorTriggeredByTechTab = false;
      return;
    }    
    currentEditMotorId = id;

    for (auto& obj_td: objsToDeleteMotors) {
      lv_obj_del(obj_td);
    }
    objsToDeleteMotors.clear();
    currentEditMotorSlidersVec.clear();
    String param_info = "Safety_threshold:\n";
    int yOffset = 45;  // Initial Y position for UI elements
    // Update the label for parameters
    lv_obj_t * label_motor_params = lv_label_create(parent);
    lv_obj_set_width(label_motor_params, 180); // Set width for better display
    lv_obj_align(label_motor_params, LV_ALIGN_TOP_LEFT, -10, yOffset);
    objsToDeleteMotors.push_back(label_motor_params);
    lv_label_set_text(label_motor_params, param_info.c_str());
    yOffset += 25;
    // Iterate over all parameters
    const auto& param = motor.safetyThreshold;
    lv_obj_t* slider = lv_slider_create(parent);
    lv_slider_set_range(slider, param.min, param.max);
    lv_slider_set_value(slider, param.currentVal, LV_ANIM_OFF);
    lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_set_size(slider, 140, 8);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 10, yOffset);
    objsToDeleteMotors.push_back(slider);
    lv_obj_set_ext_click_area(slider, 10);
    static lv_style_t style_disable;
    lv_style_init(&style_disable);
    lv_style_set_bg_color(&style_disable, HEX_DARK_GRAY);
    lv_style_set_line_color(&style_disable, HEX_DARK_GRAY);
    lv_obj_add_style(slider, &style_disable, LV_PART_INDICATOR | LV_STATE_DISABLED);
    lv_obj_add_style(slider, &style_disable, LV_PART_KNOB | LV_STATE_DISABLED);
    lv_obj_add_style(slider, &style_disable, LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_border_color(slider, HEX_BLACK,LV_PART_INDICATOR);
    lv_obj_t* minValueLabel = lv_label_create(parent);
    lv_label_set_text_fmt(minValueLabel, "%d", param.min);
    lv_obj_align(minValueLabel, LV_ALIGN_TOP_LEFT, -10, yOffset - 5);
    lv_obj_set_style_text_font(minValueLabel, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(minValueLabel);
    lv_obj_t* maxValueLabel = lv_label_create(parent);
    lv_label_set_text_fmt(maxValueLabel, "%d", param.max);
    lv_obj_align(maxValueLabel, LV_ALIGN_TOP_LEFT, 155, yOffset - 5);
    lv_obj_set_style_text_font(maxValueLabel, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(maxValueLabel);
    yOffset += 15;
    // Create a label to display the current value
    lv_obj_t* valueLabel = lv_label_create(parent);
    lv_label_set_text_fmt(valueLabel, "%d", param.currentVal);
    lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
    objsToDeleteMotors.push_back(valueLabel);
    currentEditMotorSlidersVec.push_back(slider);
    lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
    lv_obj_add_event_cb(slider, sliderEventCbUpdatedValMotor, LV_EVENT_RELEASED, NULL);
    yOffset += 20;
      lv_obj_set_user_data(slider, valueLabel);
    // If parameter is not editable, disable the slider
    if (!param.modifyPermission) {
        lv_obj_add_state(slider, LV_STATE_DISABLED);
    }
        saveBtnTechMotors = createNewBtn(
        parent,
        72,
        32,
        LV_ALIGN_TOP_MID,
        -50,
        yOffset + 8,
        "Save",
        HEX_GREEN,
        HEX_WHITE,
        &lv_font_montserrat_14);
    objsToDeleteMotors.push_back(saveBtnTechMotors);
    lv_obj_t* discardBtn = createNewBtn(
        parent,
        72,
        32,
        LV_ALIGN_TOP_MID,
        30,
        yOffset + 8,
        "Discard",
        HEX_RED,
        HEX_WHITE,
        &lv_font_montserrat_14);
    objsToDeleteMotors.push_back(discardBtn);
    yOffset += 50;
    lv_obj_t * labelMotorName = lv_label_create(parent);
    lv_obj_set_width(labelMotorName, 180); // Set width for better display
    lv_obj_align(labelMotorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorName, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(labelMotorName);
    yOffset += 20;
    lv_obj_t * labelMotorType = lv_label_create(parent);
    lv_obj_set_width(labelMotorType, 180); // Set width for better display
    lv_obj_align(labelMotorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorType, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(labelMotorType);
    yOffset += 25;
    lv_label_set_text_fmt(labelMotorName, "Name: %s", motor.name.c_str());
    lv_label_set_text_fmt(labelMotorType, "Type: %s", motor.type.c_str());
    lv_obj_t * labelMotorPin = lv_label_create(parent);
    lv_obj_set_width(labelMotorPin, 180); // Set width for better display
    lv_obj_align(labelMotorPin, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorPin, &lv_font_montserrat_14, 0);
    lv_label_set_text(labelMotorPin, "Pins:");
    objsToDeleteMotors.push_back(labelMotorPin);
    yOffset += 20;
    // Iterate over all parameters
    int i = 0;
    for (const auto& param: motor.pins) {
          lv_obj_t * labelPinType = lv_label_create(parent);
          lv_obj_set_width(labelPinType, 180); // Set width for better display
          lv_obj_align(labelPinType, LV_ALIGN_TOP_LEFT, -10, yOffset);
          lv_obj_set_style_text_font(labelPinType, &lv_font_montserrat_12, 0);
          objsToDeleteMotors.push_back(labelPinType);
          lv_label_set_text_fmt(labelPinType, "Pin Type: %s", param.type.c_str());
          yOffset += 20; 
          lv_obj_t * labelPinNumber = lv_label_create(parent);
          lv_obj_set_width(labelPinNumber, 180);
          lv_obj_align(labelPinNumber, LV_ALIGN_TOP_LEFT, -10, yOffset);
          lv_obj_set_style_text_font(labelPinNumber, &lv_font_montserrat_12, 0);
          objsToDeleteMotors.push_back(labelPinNumber);
          lv_label_set_text_fmt(labelPinNumber, "Pin Number: %d", param.pinNumber);
          yOffset += 20; 
    }
    lv_obj_add_event_cb(saveBtnTechMotors, saveBtnTechMotorClickEvent, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(saveBtnTechMotors, saveBtnTechMotorApproved, EVENT_SENSOR_CHANGED_SECC, NULL);
    lv_obj_add_event_cb(discardBtn, discardMotorBtnClickEvent, LV_EVENT_CLICKED, NULL);
    
  }
  
}

void eventViewAndEditSensors(lv_event_t* e) {
  lv_obj_t* parent = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t * obj = lv_event_get_target(e);
  int id = -1;
  if(obj){
    id = lv_dropdown_get_selected(obj);
  }
  Sensor& sensor = sensors[id];

  if (((currentEditSensorId != -1) && (currentEditSensorId != id)) || sensorTriggeredByTechTab) {
    if (checkUnsaveSensorParam().paramsId.size() != 0) {
        lv_dropdown_set_selected(obj, currentEditSensorId);
        if (sensorTriggeredByTechTab) {
          lv_tabview_set_act(techTabview, 1, LV_ANIM_ON);
        }
        lv_obj_t* currMsgBox = lv_msgbox_create(
            NULL,
            "Changes were not saved",
            "Please choose Save/Discard",
            NULL,
            true);
        lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
        sensorTriggeredByTechTab = false;
    }
    else {
      if (sensorTriggeredByTechTab) {
        sensorTriggeredByTechTab = false;
        return;
      }
      else{
        currentEditSensorId = id;
        for (auto& obj_td: objsToDeleteSensors) {
          lv_obj_del(obj_td);
        }
        objsToDeleteSensors.clear();
        currentEditSensorSlidersVec.clear();
        String param_info = "Parameters:\n";
        int yOffset = 30;  // Initial Y position for UI elements
        // Update the label for parameters
        lv_obj_t * labelSensorParams = lv_label_create(parent);
        lv_obj_set_width(labelSensorParams, 180); // Set width for better display
        lv_obj_align(labelSensorParams, LV_ALIGN_TOP_LEFT, -10, yOffset);
        objsToDeleteSensors.push_back(labelSensorParams);
        lv_label_set_text(labelSensorParams, param_info.c_str());
        yOffset += 20;
        // Iterate over all parameters
        int i = 0;
        for (const auto& param : sensor.function.parameters) {
            // Create a label for parameter name
            lv_obj_t* paramLabel = lv_label_create(parent);
            lv_label_set_text_fmt(paramLabel, "%s:", param.first.c_str());
            lv_obj_align(paramLabel, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(paramLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(paramLabel);
            yOffset += 20;
            lv_obj_t* slider = lv_slider_create(parent);
            lv_slider_set_range(slider, param.second.min, param.second.max);
            lv_slider_set_value(slider, param.second.currentVal, LV_ANIM_OFF);
            lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
            lv_obj_set_size(slider, 140, 8);
            lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 10, yOffset);
            objsToDeleteSensors.push_back(slider);
            lv_obj_set_ext_click_area(slider, 10);
            static lv_style_t style_disable;
            lv_style_init(&style_disable);
            lv_style_set_bg_color(&style_disable, HEX_DARK_GRAY);
            lv_style_set_line_color(&style_disable, HEX_DARK_GRAY);
            lv_obj_add_style(slider, &style_disable, LV_PART_INDICATOR | LV_STATE_DISABLED);
            lv_obj_add_style(slider, &style_disable, LV_PART_KNOB | LV_STATE_DISABLED);
            lv_obj_add_style(slider, &style_disable, LV_PART_MAIN | LV_STATE_DISABLED);
            lv_obj_set_style_border_color(slider, HEX_BLACK,LV_PART_INDICATOR);
            lv_obj_t* minValueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(minValueLabel, "%d", param.second.min);
            lv_obj_align(minValueLabel, LV_ALIGN_TOP_LEFT, -10, yOffset - 5);
            lv_obj_set_style_text_font(minValueLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(minValueLabel);
            lv_obj_t* maxValueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(maxValueLabel, "%d", param.second.max);
            lv_obj_align(maxValueLabel, LV_ALIGN_TOP_LEFT, 155, yOffset - 5);
            lv_obj_set_style_text_font(maxValueLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(maxValueLabel);
            yOffset += 10;
            // Create a label to display the current value
            lv_obj_t* valueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(valueLabel, "%d", param.second.currentVal);
            lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
            objsToDeleteSensors.push_back(valueLabel);
            currentEditSensorSlidersVec.push_back(slider);

            lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
            lv_obj_add_event_cb(slider, sliderEventCbUpdatedVal, LV_EVENT_RELEASED, NULL);


            yOffset += 20;
          
            lv_obj_set_user_data(slider, valueLabel);

            // If parameter is not editable, disable the slider
            if (!param.second.modifyPermission) {
                lv_obj_add_state(slider, LV_STATE_DISABLED);
            } 
            i++;  
        }
        saveBtnTechSensors = createNewBtn(
            parent,
            72,
            32,
            LV_ALIGN_TOP_MID,
            -50,
            yOffset + 8,
            "Save",
            HEX_GREEN,
            HEX_WHITE,
            &lv_font_montserrat_14);
        objsToDeleteSensors.push_back(saveBtnTechSensors);
        lv_obj_t* discardBtn = createNewBtn(
            parent,
            72,
            32,
            LV_ALIGN_TOP_MID,
            30,
            yOffset + 8,
            "Discard",
            HEX_RED,
            HEX_WHITE,
            &lv_font_montserrat_14);
        objsToDeleteSensors.push_back(discardBtn);
        yOffset += 50;
        // Create labels for sensor details
        lv_obj_t * labelSensorName = lv_label_create(parent);
        lv_obj_set_width(labelSensorName, 180); // Set width for better display
        lv_obj_align(labelSensorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorName,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorName);
        yOffset += 20;
        lv_obj_t * labelSensorStatus = lv_label_create(parent);
        lv_obj_set_width(labelSensorStatus, 180); // Set width for better display
        lv_obj_align(labelSensorStatus, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorStatus,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorStatus);
        yOffset += 20;
        lv_obj_t * labelSensorType = lv_label_create(parent);
        lv_obj_set_width(labelSensorType, 180); // Set width for better display
        lv_obj_align(labelSensorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorType,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorType);
        yOffset += 20;
        lv_label_set_text_fmt(labelSensorName, "Name: %s", sensor.name.c_str());
        lv_label_set_text_fmt(labelSensorStatus, "Status: %s", sensor.status.c_str());
        lv_label_set_text_fmt(labelSensorType, "Type: %s", sensor.type.c_str());
        lv_obj_add_event_cb(saveBtnTechSensors, saveBtnTechSensorClickEvent, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(saveBtnTechSensors, saveBtnTechSensorApproved, EVENT_SENSOR_CHANGED_SECC, NULL);
        lv_obj_add_event_cb(discardBtn, discardSensorBtnClickEvent, LV_EVENT_CLICKED, NULL);
      }    
    }

  }
  else if (currentEditSensorId == id) {
    return;
  }
  else{
        currentEditSensorId = id;
        for (auto& obj_td: objsToDeleteSensors) {
          lv_obj_del(obj_td);
        }
        objsToDeleteSensors.clear();
        currentEditSensorSlidersVec.clear();

        // Prepare formatted parameter information
        String param_info = "Parameters:\n";
        int yOffset = 30;  // Initial Y position for UI elements

        // Update the label for parameters
        lv_obj_t * labelSensorParams = lv_label_create(parent);
        lv_obj_set_width(labelSensorParams, 180); // Set width for better display
        lv_obj_align(labelSensorParams, LV_ALIGN_TOP_LEFT, -10, yOffset);
        objsToDeleteSensors.push_back(labelSensorParams);
        lv_label_set_text(labelSensorParams, param_info.c_str());

        yOffset += 20;
        // Iterate over all parameters
        int i = 0;
        for (const auto& param : sensor.function.parameters) {

            // Create a label for parameter name
            lv_obj_t* paramLabel = lv_label_create(parent);
            lv_label_set_text_fmt(paramLabel, "%s:", param.first.c_str());
            lv_obj_align(paramLabel, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(paramLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(paramLabel);

            yOffset += 20;
            lv_obj_t* slider = lv_slider_create(parent);
            lv_slider_set_range(slider, param.second.min, param.second.max);
            lv_slider_set_value(slider, param.second.currentVal, LV_ANIM_OFF);
            lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
            lv_obj_set_size(slider, 140, 8);
            lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 10, yOffset);
            objsToDeleteSensors.push_back(slider);
      
            lv_obj_set_ext_click_area(slider, 10);
            static lv_style_t style_disable;
            lv_style_init(&style_disable);
            lv_style_set_bg_color(&style_disable, HEX_DARK_GRAY);
            lv_style_set_line_color(&style_disable, HEX_DARK_GRAY);
            lv_obj_add_style(slider, &style_disable, LV_PART_INDICATOR | LV_STATE_DISABLED);
            lv_obj_add_style(slider, &style_disable, LV_PART_KNOB | LV_STATE_DISABLED);
            lv_obj_add_style(slider, &style_disable, LV_PART_MAIN | LV_STATE_DISABLED);
            lv_obj_set_style_border_color(slider, HEX_BLACK,LV_PART_INDICATOR);

            lv_obj_t* minValueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(minValueLabel, "%d", param.second.min);
            lv_obj_align(minValueLabel, LV_ALIGN_TOP_LEFT, -10, yOffset - 5);
            lv_obj_set_style_text_font(minValueLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(minValueLabel);

            lv_obj_t* maxValueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(maxValueLabel, "%d", param.second.max);
            lv_obj_align(maxValueLabel, LV_ALIGN_TOP_LEFT, 155, yOffset - 5);
            lv_obj_set_style_text_font(maxValueLabel,&lv_font_montserrat_12,0);
            objsToDeleteSensors.push_back(maxValueLabel);


            yOffset += 10;
            // Create a label to display the current value
            lv_obj_t* valueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(valueLabel, "%d", param.second.currentVal);
            lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
            objsToDeleteSensors.push_back(valueLabel);
            currentEditSensorSlidersVec.push_back(slider);

            lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
            lv_obj_add_event_cb(slider, sliderEventCbUpdatedVal, LV_EVENT_RELEASED, NULL);


            yOffset += 20;
          
            lv_obj_set_user_data(slider, valueLabel);

            // If parameter is not editable, disable the slider
            if (!param.second.modifyPermission) {
                lv_obj_add_state(slider, LV_STATE_DISABLED);
            } 
            i++;  
        }
        saveBtnTechSensors = createNewBtn(
            parent,
            72,
            32,
            LV_ALIGN_TOP_MID,
            -50,
            yOffset + 8,
            "Save",
            HEX_GREEN,
            HEX_WHITE,
            &lv_font_montserrat_14);
        objsToDeleteSensors.push_back(saveBtnTechSensors);
        lv_obj_t* discardBtn = createNewBtn(
            parent,
            72,
            32,
            LV_ALIGN_TOP_MID,
            30,
            yOffset + 8,
            "Discard",
            HEX_RED,
            HEX_WHITE,
            &lv_font_montserrat_14);
        objsToDeleteSensors.push_back(discardBtn);
        yOffset += 50;
        // Create labels for sensor details
        lv_obj_t * labelSensorName = lv_label_create(parent);
        lv_obj_set_width(labelSensorName, 180); // Set width for better display
        lv_obj_align(labelSensorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorName,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorName);
        yOffset += 20;
        lv_obj_t * labelSensorStatus = lv_label_create(parent);
        lv_obj_set_width(labelSensorStatus, 180); // Set width for better display
        lv_obj_align(labelSensorStatus, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorStatus,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorStatus);
        yOffset += 20;
        lv_obj_t * labelSensorType = lv_label_create(parent);
        lv_obj_set_width(labelSensorType, 180); // Set width for better display
        lv_obj_align(labelSensorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorType,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorType);
        yOffset += 20;
        lv_label_set_text_fmt(labelSensorName, "Name: %s", sensor.name.c_str());
        lv_label_set_text_fmt(labelSensorStatus, "Status: %s", sensor.status.c_str());
        lv_label_set_text_fmt(labelSensorType, "Type: %s", sensor.type.c_str());
        lv_obj_add_event_cb(saveBtnTechSensors, saveBtnTechSensorClickEvent, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(saveBtnTechSensors, saveBtnTechSensorApproved, EVENT_SENSOR_CHANGED_SECC, NULL);
        lv_obj_add_event_cb(discardBtn, discardSensorBtnClickEvent, LV_EVENT_CLICKED, NULL);
  }
}

void techTabviewEventCb(lv_event_t* e) {
  uint16_t newTabId = lv_tabview_get_tab_act(techTabview);
  if (currTechTabviewId != newTabId) {
    switch (currTechTabviewId) {
      case 0:
        motorTriggeredByTechTab = true;
        lv_event_send(dropdownMotors, LV_EVENT_VALUE_CHANGED, techTabMotors);
        break;
      case 1:
        sensorTriggeredByTechTab = true;
        lv_event_send(dropdownSensors, LV_EVENT_VALUE_CHANGED, techTabSensors);
        break;
      case 2:
        break;
      default:
        break;
    }

    newTabId = lv_tabview_get_tab_act(techTabview);
    currTechTabviewId = newTabId;
  }
  else{
    currTechTabviewId = newTabId;
  }
}

void createControlsForTech(lv_obj_t* parent) {
      techTabview = lv_tabview_create(parent, LV_DIR_RIGHT, 80);
    lv_obj_set_style_bg_color(techTabview, HEX_MEDIUM_GRAY, 0);
    lv_obj_set_style_bg_opa(techTabview, LV_OPA_COVER, 0);
    lv_obj_t * techTabBtns = lv_tabview_get_tab_btns(techTabview);
    lv_obj_set_style_bg_color(techTabBtns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(techTabBtns, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(techTabBtns, LV_BORDER_SIDE_LEFT, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(techTabBtns, LV_OPA_COVER, 0);
    techTabMotors = lv_tabview_add_tab(techTabview, "Motors\n" LV_SYMBOL_SETTINGS );
    techTabSensors = lv_tabview_add_tab(techTabview,  "Sensors\n" LV_SYMBOL_SETTINGS);
    techTabMotorsActivity = lv_tabview_add_tab(techTabview, "Motors\nActivity");  
    lv_obj_clear_flag(lv_tabview_get_content(techTabview), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(techTabview, techTabviewEventCb, LV_EVENT_VALUE_CHANGED, NULL);
    dropdownMotors = createMotorsTabTech(techTabMotors);
    dropdownSensors = createSensorsTabTech(techTabSensors);
    dropdownMotorsActivity = createMotorsTabTech(techTabMotorsActivity);

    //motorsActivity(techTabMotorsActivity);
    lv_obj_add_event_cb(dropdownSensors, eventViewAndEditSensors, LV_EVENT_VALUE_CHANGED, techTabSensors);
    lv_obj_add_event_cb(dropdownMotors, eventViewEditMotors, LV_EVENT_VALUE_CHANGED, techTabMotors);

}

void setup()
{
    Serial.begin(115200);
    delay(1500); // milisec
    xTaskCreate(StartBLEServer,"Initialize", STACK_SIZE, nullptr, 2, nullptr);
    // initial atomic flags
    canPlayGesture.test_and_set();
    notRemoveBox.test_and_set();
    // hasClient.test_and_set();
    // Init Display
    gfx->begin(80000000);
    #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcSetup(0, 2000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 255); // Screen brightness can be modified by adjusting this parameter.1 (0-255)
    #endif
    lv_init();
    touchInit();
    screenWidth = gfx->width();
    screenHeight = gfx->height();
    #ifdef ESP32
    dispDrawBuf = (lv_color_t *)heap_caps_malloc(
        sizeof(lv_color_t) * screenWidth * screenHeight / 2,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    #else
    dispDrawBuf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 2);
    #endif
   
    if (!dispDrawBuf) {
      Serial.println("LVGL dispDrawBuf allocate failed!");
      return;
    }
    lv_disp_draw_buf_init(&drawBuf, dispDrawBuf, NULL, screenWidth * screenHeight / 2);

    // Initialize the display with handlers
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = myDispFlush;
    disp_drv.draw_buf = &drawBuf;
    lv_disp_drv_register(&disp_drv);

    // Initialize the (dummy) input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = myTouchpadRead;
    lv_indev_drv_register(&indev_drv);
    
    // Initial setup screen for setting isUser
    msgBoxParent = lv_obj_create(NULL);
    lv_obj_clear_flag(msgBoxParent, LV_OBJ_FLAG_SCROLLABLE);
    notFinishUpdateSensors.test_and_set();
    searchClientBLEScreen();

    readYamlFromProtScreenFunction();
    setupWelcomeScreen(); // Show the welcome screen
    
    Serial.println("Setup done");
}

void readYamlFromProtScreenFunction() {
    readYamlFromProtScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(readYamlFromProtScreen, HEX_WHITE, 0); // Pink background

    lv_obj_t * label1 = lv_label_create(readYamlFromProtScreen);
    lv_label_set_recolor(label1, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label1, "#00007f Reading YAML File. \n Please Wait... #");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label1,&lv_font_montserrat_28,0);
    lv_obj_add_event_cb(readYamlFromProtScreen, loadYamlStep, LV_EVENT_CLICKED, NULL);
  
} 

void setupWelcomeScreen() {
    welcomeScreen = lv_obj_create(NULL);
    lv_scr_load_anim(welcomeScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    lv_obj_set_style_bg_color(welcomeScreen, HEX_WHITE, 0); // Pink background

    lv_obj_t * label1 = lv_label_create(welcomeScreen);
    lv_label_set_recolor(label1, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label1, "#000099 S##000066 P##00007f M##0000b2 T#");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_font(label1,&lv_font_montserrat_48,0);


    lv_obj_t * label2 = lv_label_create(welcomeScreen);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
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

void loadYamlStep(lv_event_t* e) {
  if (hasClient.test_and_set()) {
      if (sendYamlRequest) {
        SendNotifyToClient("Please send YAML data", YAML_REQ, pCharacteristic);
        Serial.println("Sent yaml request");
        sendYamlRequest = false;
      }
      if (isYmlSensorsReady) {
          sensors.clear(); // making sure to clear demo yaml data before replacong it with real data
          splitSensorsField((char*)*pointerToSensorBuff);
          isYmlSensorsReady = false;
          free(*pointerToSensorBuff);
      }

      if (isYmlMotorsReady) {
        motors.clear(); // making sure to clear demo yaml data before replacong it with real data
        splitMotorsField((char*)*pointerToMotorsBuff);
        isYmlMotorsReady = false;
        free(*pointerToMotorsBuff);
      }

      if (isYmlFunctionsReady) {
        functions.clear(); // making sure to clear demo yaml data before replacong it with real data
        splitFunctionsField((char*)*pointerToFuncBuff);
        isYmlFunctionsReady = false;
        free(*pointerToFuncBuff);
      }

      if (isYmlGeneralReady) {
        generalEntries.clear(); // making sure to clear demo yaml data before replacong it with real data
        splitGeneralField((char*)*pointerToGeneralBuff);
        isYmlGeneralReady = false;
        free(*pointerToGeneralBuff);
        yamlStructsReady = true;
      }

      if (yamlStructsReady) {
        yamlStructsReady = false;
        xTaskCreate(BLENotifyTask, "BLE Notify Task", 2048, NULL, 2, &bleNotifyTaskHandle);
        pinMode(buttonPin, INPUT_PULLUP);
        attachInterrupt(buttonPin, buttonPress, RISING);
        setupInitialUserScreen();
      }
      
      else{
        delay(100);
        loadYamlStep(e);
      }
  }
  else{
    hasClient.clear();
    lv_scr_load_anim(searchBleScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
  }
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
    lv_obj_set_style_bg_color(searchBleScreen, HEX_WHITE, 0); // Pink background

    lv_obj_t * label1 = lv_label_create(searchBleScreen);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label1, 250);
    lv_label_set_recolor(label1, true);                      /*Enable re-coloring by commands in the text*/
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


    // lv_obj_t * label_search = lv_obj_get_child(searchBleBtn, 0);
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

  // Create a label
  lv_obj_t* label = lv_label_create(initialUserScreen);
  lv_label_set_text(label, "Select Mode:");
  lv_obj_align(label,LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_font(label,&lv_font_montserrat_18,0);

  // Create a button matrix for user mode selection
  static char* btnm_map[] = {"User", "\n", "Tech", "Debug", ""}; // The last element must be an empty string
  int num_pointer = sizeof(btnm_map) / sizeof(btnm_map[0]);

  static char*** map_ptr = (char***)malloc(sizeof(char**));
  *map_ptr = btnm_map;

  lv_obj_t* btnm = createNewMatrixBtnChooseOne(
      initialUserScreen,
      map_ptr,
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

    //            is_user = false;            is_tech = true;
    if (!isUser) {
      techTab = lv_tabview_add_tab(tabview, "Tech");
      lv_obj_set_style_bg_color(techTab, HEX_MEDIUM_GRAY, 0);
      lv_obj_set_style_bg_opa(techTab, LV_OPA_COVER, 0);
    }
    if  (!isUser && !isTech){
        debugTab = lv_tabview_add_tab(tabview, "Debug");
    }

    createControlsForMain(homeTab);
    createControlsForStat(statTab);
    createControlsForSetup(setupTab);
    if (!isUser) {
      createControlsForTech(techTab);
    }
    if  (!isUser && !isTech){
    
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

// //delete debug screens objects
// void deleteDebug(){
//   if (chartTimer) {
//     lv_timer_del(chartTimer); // Stop the timer
//     chartTimer = NULL;
//   }
//   delay(200);
//   if(debugLabel){
//     lv_obj_del(debugLabel);
//     debugLabel = NULL;
//   }
//   if (chart) {// Delete the chart
//     lv_obj_del(chart);
//     chart = NULL;
//   }
//   if (closeChartBTN) {// Delete the close button
//     lv_obj_del(closeChartBTN);
//     closeChartBTN = NULL;
//   }
//   if(dropdownMotorsObj){
//     lv_obj_del(dropdownMotorsObj);
//     dropdownMotorsObj = NULL;
//   }
//   if(dropdownSensorsObj){
//     lv_obj_del(dropdownSensorsObj);
//     dropdownSensorsObj = NULL;
//   }
//   if (TabviewObjDebugMode){
//     lv_obj_del(TabviewObjDebugMode);
//     TabviewObjDebugMode = NULL;
//   }
//   if(debugTab){
//     lv_obj_del(debugTab);
//     debugTab = NULL;
//   }
// }

void loop(){
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
