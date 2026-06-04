#include "UIUserMode.h"
#include "UIShared.h"
#include "UINavigation.h"
#include "ConfigParams.h"
#include <SharedYamlParser.h>
#include "BLEServer.h"
#include <Arduino.h>

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
    lv_obj_add_event_cb(returnBtn, returnToMain, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_text_align(returnBtn, LV_TEXT_ALIGN_CENTER, 0);
    
    lv_obj_t * label_home = lv_label_create(parent);
    lv_label_set_recolor(label_home, true);
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
        lv_obj_set_style_text_color(label_home_BLE_1, HEX_ROYAL_BLUE, 0);
        lv_obj_set_style_text_color(label_home_BLE_2, HEX_GREEN, 0);
    }
    else{
        lv_label_set_text(label_home_BLE_2,  LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(label_home_BLE_2, HEX_RED, 0);
        hasClient.clear();
    }
    
    lv_obj_set_style_text_align(label_home_BLE_1, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(label_home_BLE_2, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(label_home_BLE_1, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_obj_align(label_home_BLE_2, LV_ALIGN_TOP_LEFT, 22, 80);
    lv_obj_set_style_text_font(label_home_BLE_1,&lv_font_montserrat_22,0);
    lv_obj_set_style_text_font(label_home_BLE_2,&lv_font_montserrat_22,0);

    #define MAX_GESTURES_SUPPORTED 16
    int numGest = getGestNum();
    int numFunctionTotal = functions.size();

    if (numGest > MAX_GESTURES_SUPPORTED) {
      Serial.println("WARNING: numGest exceeds MAX_GESTURES_SUPPORTED");
      numGest = MAX_GESTURES_SUPPORTED;
    }
    lv_obj_t* gesturesMatrix[MAX_GESTURES_SUPPORTED];
    int maxInRow = 2;

    int j = 0;
    for (int i = 0; i < numFunctionTotal; i++) {
      if (j >= numGest) break;
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

    lv_obj_t* labelGest = lv_label_create(parent);
    lv_label_set_text(labelGest, "Gestures");
    lv_obj_align(labelGest,LV_ALIGN_TOP_RIGHT, -64, -5);
    lv_obj_set_style_text_font(labelGest,&lv_font_montserrat_18,0);
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
  lv_obj_t* titleLabel = lv_label_create(parent); 
  lv_label_set_text(titleLabel, "Sensors State:"); 
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10); 
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_18, 0); 

  int yOffset = 40;

  for (const auto& sensor : sensors) { 
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);

      if (sensor.status.equalsIgnoreCase("ON")) {
          lv_obj_t* sensorNameLabel = lv_label_create(parent); 
          lv_label_set_text(sensorNameLabel, (display_name + ":").c_str()); 
          lv_obj_align(sensorNameLabel, LV_ALIGN_TOP_LEFT, 15, yOffset); 
          lv_obj_set_style_text_font(sensorNameLabel, &lv_font_montserrat_16, 0); 

          lv_obj_t* sensorStatusLabel = lv_label_create(parent); 
          lv_obj_align(sensorStatusLabel, LV_ALIGN_TOP_RIGHT, -15, yOffset); 
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);
          
          lv_label_set_text(sensorStatusLabel, "ON " LV_SYMBOL_OK); 
          lv_obj_set_style_text_color(sensorStatusLabel, HEX_GREEN, 0);
          yOffset += 28;
      } 
  }
  
  for (const auto& sensor : sensors){
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);

    if (sensor.status.equalsIgnoreCase("OFF")) {
          lv_obj_t* sensor_name_label = lv_label_create(parent); 
          lv_label_set_text(sensor_name_label, (display_name + ":").c_str()); 
          lv_obj_align(sensor_name_label, LV_ALIGN_TOP_LEFT, 15, yOffset); 
          lv_obj_set_style_text_font(sensor_name_label, &lv_font_montserrat_16, 0); 

          lv_obj_t* sensorStatusLabel = lv_label_create(parent); 
          lv_obj_align(sensorStatusLabel, LV_ALIGN_TOP_RIGHT, -15, yOffset); 
          lv_obj_set_style_text_font(sensorStatusLabel, &lv_font_montserrat_16, 0);
      
          lv_label_set_text(sensorStatusLabel, "OFF " LV_SYMBOL_CLOSE); 
          lv_obj_set_style_text_color(sensorStatusLabel,HEX_RED, 0);

          yOffset += 28;
    }
  }
}

void discardBtnClickEvent(lv_event_t * e) {
  for (size_t i=0; i< sensors.size(); i++) {
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

void saveNewSwitchBtnmToStruct() {
  for (size_t i=0; i< sensors.size(); i++) {
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

void createControlsForSetup(lv_obj_t* parent) {
  lv_obj_t* titleLabel = lv_label_create(parent); 
  lv_label_set_text(titleLabel, "Sensors Setup:"); 
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10); 
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_18, 0); 

  int yOffset = 40;

  static lv_style_t style_indic_on;
  static lv_style_t style_indic_off;

  lv_style_init(&style_indic_on);
  lv_style_set_bg_color(&style_indic_on, HEX_DARK_BLUE);
  lv_style_set_bg_opa(&style_indic_on, LV_OPA_COVER);

  lv_style_init(&style_indic_off);
  lv_style_set_bg_color(&style_indic_off,  HEX_LIGHT_GRAY_2); 
  lv_style_set_bg_opa(&style_indic_off, LV_OPA_COVER);

  for (const auto& sensor : sensors) { 
      String display_name = sensor.name;
      display_name.replace("_", " ");
      display_name[0] = toupper(display_name[0]);
      
      lv_obj_t* sensor_name_label = lv_label_create(parent); 
      lv_label_set_text(sensor_name_label, (display_name + ":").c_str()); 
      lv_obj_align(sensor_name_label, LV_ALIGN_TOP_LEFT, 15, yOffset+5); 
      lv_obj_set_style_text_font(sensor_name_label, &lv_font_montserrat_16, 0); 

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
