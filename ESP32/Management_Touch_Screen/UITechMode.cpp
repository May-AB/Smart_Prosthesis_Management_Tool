#include "UITechMode.h"
#include "UIShared.h"
#include "UINavigation.h"
#include "ConfigParams.h"
#include "SharedYamlParser.h"
#include "BLEServer.h"
#include <Arduino.h>

void sliderEventCbAnim(lv_event_t* e) {
  lv_obj_t* valLabel = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t * slider = lv_event_get_target(e);
  lv_label_set_text(valLabel, String(lv_slider_get_value(slider)).c_str());
}

void sliderEventCbUpdatedVal(lv_event_t* e) {
  // Event callback placeholder
}

void sliderEventCbUpdatedValMotor(lv_event_t* e) {
  // Event callback placeholder
}

ReturnUnsavedParam checkUnsaveSensorParam() {
  ReturnUnsavedParam retStruct;
  std::vector<int> retParamsId;
  std::vector<int> retNewVals;
  retStruct.itemId = currentEditSensorId;
  int i = 0;
  if (currentEditSensorSlidersVec.size() > 0 && currentEditSensorId != -1) {
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

ReturnUnsavedParam checkUnsaveMotorParam() {
  ReturnUnsavedParam retStruct;
  std::vector<int> retParamsId;
  std::vector<int> retNewVals;
  retStruct.itemId = currentEditMotorId;
  int i = 0;
  if (currentEditMotorSlidersVec.size() > 0 && currentEditMotorId != -1) {
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
    
    lv_meter_scale_t * scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);
    lv_meter_indicator_t * indic;
    
    indic = lv_meter_add_arc(meter, scale, 3, HEX_DARK_BLUE, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);
    
    indic = lv_meter_add_scale_lines(meter, scale, HEX_DARK_BLUE, HEX_DARK_BLUE,false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);
    
    indic = lv_meter_add_arc(meter, scale, 3, HEX_RED, 0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    
    indic = lv_meter_add_scale_lines(meter, scale, HEX_RED, HEX_RED, false,0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    
    indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);
    
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

void discardSensorBtnClickEvent(lv_event_t * e) {
  ReturnUnsavedParam unsaveStruct = checkUnsaveSensorParam();
  if (unsaveStruct.paramsId.size() > 0 && currentEditSensorId != -1) {
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
  ReturnUnsavedParam unsaveTsh = checkUnsaveMotorParam();
  if (unsaveTsh.paramsId.size() > 0 && currentEditMotorId != -1) {
    lv_slider_set_value(
        currentEditMotorSlidersVec[0],
        motors[currentEditMotorId].safetyThreshold.currentVal,
        LV_ANIM_ON);
    lv_event_send(currentEditMotorSlidersVec[0], LV_EVENT_VALUE_CHANGED, NULL);
  }
}

void saveNewMotorValToStruct(ReturnUnsavedParam motorThsStruct) {
    if (motorThsStruct.itemId != -1) {
      motors[motorThsStruct.itemId].safetyThreshold.currentVal = motorThsStruct.newVals[0];
    }
}

void saveNewSensorsValToStruct(ReturnUnsavedParam sensorsStruct) {
    if (sensorsStruct.itemId == -1) return;
    size_t i = 0;
    int j = 0;
    for(auto& [name, param] : sensors[sensorsStruct.itemId].function.parameters){
      if(i < sensorsStruct.paramsId.size() && sensorsStruct.paramsId[i] == j){
        param.currentVal = sensorsStruct.newVals[i];
        i++;
      }
      j++;
    }
}

void saveBtnTechMotorApproved(lv_event_t * e) {
  if (sendNewSwitchesMsgBox) {
    lv_msgbox_close(sendNewSwitchesMsgBox);
    sendNewSwitchesMsgBox = NULL;
  }
  ReturnUnsavedParam unsaveMotorThs = checkUnsaveMotorParam();
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
  ReturnUnsavedParam unsaveSensors = checkUnsaveSensorParam();
  saveNewSensorsValToStruct(unsaveSensors);

  lv_obj_t* currMsgBox =
      lv_msgbox_create(NULL, LV_SYMBOL_OK, "Changes have been saved.\nProthesis is updated.", NULL, true);
  lv_obj_align(currMsgBox, LV_ALIGN_CENTER, 0, 0); 
}

void saveBtnTechMotorClickEvent(lv_event_t * e) {
    ReturnUnsavedParam unsaveMotorThs = checkUnsaveMotorParam();

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

        SendMotorParamChangeReq(unsaveMotorThs.itemId, unsaveMotorThs.newVals);

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
    ReturnUnsavedParam unsaveSensors = checkUnsaveSensorParam();

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
            unsaveSensors.itemId,
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

void eventViewEditMotors(lv_event_t* e){
  lv_obj_t* parent = (lv_obj_t*)lv_event_get_user_data(e);
  lv_obj_t * obj = lv_event_get_target(e);
  int id = -1;
  if(obj){
    id = lv_dropdown_get_selected(obj);
  }
  if (id == -1) return;
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
      for (auto& obj_td: objsToDeleteMotors) {
        lv_obj_del(obj_td);
      }
      objsToDeleteMotors.clear();
      currentEditMotorSlidersVec.clear();
      String param_info = "Safety_threshold:\n";
      int yOffset = 45;
      
      lv_obj_t * label_motor_params = lv_label_create(parent);
      lv_obj_set_width(label_motor_params, 180);
      lv_obj_align(label_motor_params, LV_ALIGN_TOP_LEFT, -10, yOffset);
      objsToDeleteMotors.push_back(label_motor_params);
      lv_label_set_text(label_motor_params, param_info.c_str());
      yOffset += 25;
      
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
      
      lv_obj_t* valueLabel = lv_label_create(parent);
      lv_label_set_text_fmt(valueLabel, "%d", param.currentVal);
      lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
      objsToDeleteMotors.push_back(valueLabel);
      currentEditMotorSlidersVec.push_back(slider);
      lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
      lv_obj_add_event_cb(slider, sliderEventCbUpdatedValMotor, LV_EVENT_RELEASED, NULL);
      yOffset += 20;
      lv_obj_set_user_data(slider, valueLabel);
      
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
      lv_obj_set_width(labelMotorName, 180);
      lv_obj_align(labelMotorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorName, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(labelMotorName);
      yOffset += 20;
      
      lv_obj_t * labelMotorType = lv_label_create(parent);
      lv_obj_set_width(labelMotorType, 180);
      lv_obj_align(labelMotorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorType, &lv_font_montserrat_12, 0);
      objsToDeleteMotors.push_back(labelMotorType);
      yOffset += 25;
      
      lv_label_set_text_fmt(labelMotorName, "Name: %s", motor.name.c_str());
      lv_label_set_text_fmt(labelMotorType, "Type: %s", motor.type.c_str());
      
      lv_obj_t * labelMotorPin = lv_label_create(parent);
      lv_obj_set_width(labelMotorPin, 180);
      lv_obj_align(labelMotorPin, LV_ALIGN_TOP_LEFT, -10, yOffset);
      lv_obj_set_style_text_font(labelMotorPin, &lv_font_montserrat_14, 0);
      lv_label_set_text(labelMotorPin, "Pins:");
      objsToDeleteMotors.push_back(labelMotorPin);

      yOffset += 20;
      for (const auto& pin: motor.pins) {
            lv_obj_t * labelPinType = lv_label_create(parent);
            lv_obj_set_width(labelPinType, 180);
            lv_obj_align(labelPinType, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(labelPinType, &lv_font_montserrat_12, 0);
            objsToDeleteMotors.push_back(labelPinType);
            lv_label_set_text_fmt(labelPinType, "Pin Type: %s", pin.type.c_str());
            yOffset += 20; 

            lv_obj_t * labelPinNumber = lv_label_create(parent);
            lv_obj_set_width(labelPinNumber, 180);
            lv_obj_align(labelPinNumber, LV_ALIGN_TOP_LEFT, -10, yOffset);
            lv_obj_set_style_text_font(labelPinNumber, &lv_font_montserrat_12, 0);
            objsToDeleteMotors.push_back(labelPinNumber);
            lv_label_set_text_fmt(labelPinNumber, "Pin Number: %d", pin.pinNumber);
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
    int yOffset = 45;
    
    lv_obj_t * label_motor_params = lv_label_create(parent);
    lv_obj_set_width(label_motor_params, 180);
    lv_obj_align(label_motor_params, LV_ALIGN_TOP_LEFT, -10, yOffset);
    objsToDeleteMotors.push_back(label_motor_params);
    lv_label_set_text(label_motor_params, param_info.c_str());
    yOffset += 25;
    
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
    
    lv_obj_t* valueLabel = lv_label_create(parent);
    lv_label_set_text_fmt(valueLabel, "%d", param.currentVal);
    lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
    objsToDeleteMotors.push_back(valueLabel);
    currentEditMotorSlidersVec.push_back(slider);
    lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
    lv_obj_add_event_cb(slider, sliderEventCbUpdatedValMotor, LV_EVENT_RELEASED, NULL);
    yOffset += 20;
    lv_obj_set_user_data(slider, valueLabel);
    
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
    lv_obj_set_width(labelMotorName, 180);
    lv_obj_align(labelMotorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorName, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(labelMotorName);
    yOffset += 20;
    
    lv_obj_t * labelMotorType = lv_label_create(parent);
    lv_obj_set_width(labelMotorType, 180);
    lv_obj_align(labelMotorType, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorType, &lv_font_montserrat_12, 0);
    objsToDeleteMotors.push_back(labelMotorType);
    yOffset += 25;
    
    lv_label_set_text_fmt(labelMotorName, "Name: %s", motor.name.c_str());
    lv_label_set_text_fmt(labelMotorType, "Type: %s", motor.type.c_str());
    
    lv_obj_t * labelMotorPin = lv_label_create(parent);
    lv_obj_set_width(labelMotorPin, 180);
    lv_obj_align(labelMotorPin, LV_ALIGN_TOP_LEFT, -10, yOffset);
    lv_obj_set_style_text_font(labelMotorPin, &lv_font_montserrat_14, 0);
    lv_label_set_text(labelMotorPin, "Pins:");
    objsToDeleteMotors.push_back(labelMotorPin);
    yOffset += 20;
    
    for (const auto& pin: motor.pins) {
          lv_obj_t * labelPinType = lv_label_create(parent);
          lv_obj_set_width(labelPinType, 180);
          lv_obj_align(labelPinType, LV_ALIGN_TOP_LEFT, -10, yOffset);
          lv_obj_set_style_text_font(labelPinType, &lv_font_montserrat_12, 0);
          objsToDeleteMotors.push_back(labelPinType);
          lv_label_set_text_fmt(labelPinType, "Pin Type: %s", pin.type.c_str());
          yOffset += 20; 
          
          lv_obj_t * labelPinNumber = lv_label_create(parent);
          lv_obj_set_width(labelPinNumber, 180);
          lv_obj_align(labelPinNumber, LV_ALIGN_TOP_LEFT, -10, yOffset);
          lv_obj_set_style_text_font(labelPinNumber, &lv_font_montserrat_12, 0);
          objsToDeleteMotors.push_back(labelPinNumber);
          lv_label_set_text_fmt(labelPinNumber, "Pin Number: %d", pin.pinNumber);
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
  if (id == -1) return;
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
        int yOffset = 30;
        
        lv_obj_t * labelSensorParams = lv_label_create(parent);
        lv_obj_set_width(labelSensorParams, 180);
        lv_obj_align(labelSensorParams, LV_ALIGN_TOP_LEFT, -10, yOffset);
        objsToDeleteSensors.push_back(labelSensorParams);
        lv_label_set_text(labelSensorParams, param_info.c_str());
        yOffset += 20;
        
        int i = 0;
        for (const auto& param : sensor.function.parameters) {
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
            
            lv_obj_t* valueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(valueLabel, "%d", param.second.currentVal);
            lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
            objsToDeleteSensors.push_back(valueLabel);
            currentEditSensorSlidersVec.push_back(slider);

            lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
            lv_obj_add_event_cb(slider, sliderEventCbUpdatedVal, LV_EVENT_RELEASED, NULL);

            yOffset += 20;
            lv_obj_set_user_data(slider, valueLabel);

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
        
        lv_obj_t * labelSensorName = lv_label_create(parent);
        lv_obj_set_width(labelSensorName, 180);
        lv_obj_align(labelSensorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorName,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorName);
        yOffset += 20;
        
        lv_obj_t * labelSensorStatus = lv_label_create(parent);
        lv_obj_set_width(labelSensorStatus, 180);
        lv_obj_align(labelSensorStatus, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorStatus,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorStatus);
        yOffset += 20;
        
        lv_obj_t * labelSensorType = lv_label_create(parent);
        lv_obj_set_width(labelSensorType, 180);
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

        String param_info = "Parameters:\n";
        int yOffset = 30;

        lv_obj_t * labelSensorParams = lv_label_create(parent);
        lv_obj_set_width(labelSensorParams, 180);
        lv_obj_align(labelSensorParams, LV_ALIGN_TOP_LEFT, -10, yOffset);
        objsToDeleteSensors.push_back(labelSensorParams);
        lv_label_set_text(labelSensorParams, param_info.c_str());

        yOffset += 20;
        int i = 0;
        for (const auto& param : sensor.function.parameters) {
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
            lv_obj_t* valueLabel = lv_label_create(parent);
            lv_label_set_text_fmt(valueLabel, "%d", param.second.currentVal);
            lv_obj_align(valueLabel, LV_ALIGN_TOP_LEFT, 75, yOffset);
            objsToDeleteSensors.push_back(valueLabel);
            currentEditSensorSlidersVec.push_back(slider);

            lv_obj_add_event_cb(slider, sliderEventCbAnim, LV_EVENT_VALUE_CHANGED, valueLabel);
            lv_obj_add_event_cb(slider, sliderEventCbUpdatedVal, LV_EVENT_RELEASED, NULL);

            yOffset += 20;
            lv_obj_set_user_data(slider, valueLabel);

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
        
        lv_obj_t * labelSensorName = lv_label_create(parent);
        lv_obj_set_width(labelSensorName, 180);
        lv_obj_align(labelSensorName, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorName,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorName);
        yOffset += 20;
        
        lv_obj_t * labelSensorStatus = lv_label_create(parent);
        lv_obj_set_width(labelSensorStatus, 180);
        lv_obj_align(labelSensorStatus, LV_ALIGN_TOP_LEFT, -10, yOffset);
        lv_obj_set_style_text_font(labelSensorStatus,&lv_font_montserrat_12,0);
        objsToDeleteSensors.push_back(labelSensorStatus);
        yOffset += 20;
        
        lv_obj_t * labelSensorType = lv_label_create(parent);
        lv_obj_set_width(labelSensorType, 180);
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

lv_obj_t* createMotorsTabTech(lv_obj_t* parent){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char options[256];
  getOptionsString(options, sizeof(options), true);
  lv_dropdown_set_text(dropdown, "Motor");
  lv_dropdown_set_options(dropdown,options);
  lv_dropdown_set_selected_highlight(dropdown, true);
  return dropdown;
}

lv_obj_t* createSensorsTabTech(lv_obj_t* parent){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char options[256];
  getOptionsString(options, sizeof(options), false);
  lv_dropdown_set_options(dropdown,options);
  lv_dropdown_set_text(dropdown, "Sensor");
  lv_dropdown_set_selected_highlight(dropdown, true);
  return dropdown;
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

    lv_obj_add_event_cb(dropdownSensors, eventViewAndEditSensors, LV_EVENT_VALUE_CHANGED, techTabSensors);
    lv_obj_add_event_cb(dropdownMotors, eventViewEditMotors, LV_EVENT_VALUE_CHANGED, techTabMotors);
}

