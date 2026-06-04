#include "UIDebugMode.h"
#include "UIShared.h"
#include "ConfigParams.h"
#include "SharedYamlParser.h"
#include "BLEServer.h"
#include <Arduino.h>

static int getSensorValue() {
    return lv_rand(10, 90);
}

static void updateChartReq(lv_timer_t *t) {
  int* arr = static_cast<int*>(t->user_data);
  char msgToSend[MAX_MSG_LEN];
  String isMotorStr    = String(arr[0]);
  String hardwareIdStr = String(arr[1]);
  int totalLen = (int)isMotorStr.length() + 1 + (int)hardwareIdStr.length();
  int ind = 0;
  memcpy(&msgToSend[ind], isMotorStr.c_str(),    isMotorStr.length());    ind += isMotorStr.length();
  msgToSend[ind++] = '|';
  memcpy(&msgToSend[ind], hardwareIdStr.c_str(), hardwareIdStr.length()); ind += hardwareIdStr.length();
  msgToSend[ind] = '\0';
  SendNotifyToClient(msgToSend, READ_REQ, pCharacteristic);
}

static void updateChart(lv_timer_t *t) {
    lv_chart_set_next_value(chart, ser, getSensorValue());
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

// Forward prototype for close callback
static void closeChartEventCB(lv_event_t * e);

static void showChartEventCB(bool isMotor, int id) {
    lv_obj_add_flag(dropdownMotorsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dropdownSensorsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(TabviewObjDebugMode, LV_OBJ_FLAG_HIDDEN);
    chart = lv_chart_create(debugTab);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_size(chart, 280, 125);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_chart_set_point_count(chart, numPoints);
    ser = lv_chart_add_series(chart, HEX_DARK_BLUE, LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);    
    static int arr[2];
    arr[0] = (int)isMotor;
    arr[1] = id;

    if (isDemoYaml.test_and_set()) {
      if (chartTimer) {
        lv_timer_del(chartTimer);
        chartTimer = NULL;
      }
      chartTimer = lv_timer_create(updateChart, 200, static_cast<void*>(arr));
    } else {
      isDemoYaml.clear();
      chartTimer = lv_timer_create(updateChartReq, 200, static_cast<void*>(arr));
    }
    closeChartBTN = lv_btn_create(debugTab);
    lv_obj_set_size(closeChartBTN, 100, 25);
    lv_obj_align(closeChartBTN, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(closeChartBTN, HEX_DARK_BLUE, 0);
    lv_obj_t * label = lv_label_create(closeChartBTN);
    lv_label_set_text(label, "Close Chart");
    lv_obj_center(label);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_12,0);
    lv_obj_add_event_cb(closeChartBTN, closeChartEventCB, LV_EVENT_CLICKED, NULL);
    debugLabel = lv_label_create(debugTab); 
    lv_label_set_text(debugLabel, selectedTextToTitle);
    lv_obj_align(debugLabel, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_set_style_text_font(debugLabel, &lv_font_montserrat_18, 0);
}

static void closeChartEventCB(lv_event_t * e) {
  if (chartTimer) {
    lv_timer_del(chartTimer);
    chartTimer = NULL;
  }
  delay(200);
  if (chart) {
    lv_obj_del(chart);
    chart = NULL;
  }
  if (closeChartBTN) {
    lv_obj_del(closeChartBTN);
    closeChartBTN = NULL;
  }
  ser = NULL;
  if(debugLabel){
    lv_obj_del(debugLabel);
    debugLabel = NULL;
  }
  lv_obj_clear_flag(dropdownMotorsObj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(dropdownSensorsObj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(TabviewObjDebugMode, LV_OBJ_FLAG_HIDDEN);
}

static void showDropdownChartCB(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    char selected_text[32];
    lv_dropdown_get_selected_str(dropdown, selected_text, sizeof(selected_text));
    strcpy(selectedTextToTitle, selected_text);
    String selectedTextStr = String(selected_text);
    bool isMotor = (dropdown == dropdownMotorsObj);
    size_t id = 0;

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

lv_obj_t* createDropdownsDebugMode(lv_obj_t* parent, bool isMotors){
  lv_obj_t * dropdown = lv_dropdown_create(parent);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, -10, -10);
  char options[256];
  getOptionsString(options, sizeof(options), isMotors);
  if(isMotors){
    lv_dropdown_set_text(dropdown, "Motor");
  } else{
    lv_dropdown_set_text(dropdown, "Sensor");
  }
  lv_dropdown_set_options(dropdown, options);
  lv_dropdown_set_selected_highlight(dropdown, true);
  lv_obj_add_event_cb(dropdown, showDropdownChartCB, LV_EVENT_VALUE_CHANGED, NULL);
  return dropdown;
}

void createDebugModeControls(lv_obj_t* parent) {
    TabviewObjDebugMode = lv_tabview_create(parent, LV_DIR_RIGHT, 70);
    lv_obj_set_style_bg_color(TabviewObjDebugMode, HEX_MEDIUM_GRAY, 0);
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
    dropdownSensorsObj = createDropdownsDebugMode(debug_tab_sensors, false);
}
