#include <lvgl.h>
#include <Arduino_GFX_Library.h>

#include "ConfigParams.h"
#include "Touch.h"
#include "BLEServer.h"
#include "UINavigation.h"

void setup()
{
    Serial.begin(115200);
    delay(1500); // milisec
    xTaskCreatePinnedToCore(StartBLEServer, "BLEServer", STACK_SIZE, nullptr, 2, nullptr, 0);
    canPlayGesture.test_and_set();
    notRemoveBox.test_and_set();
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
    setupWelcomeScreen();
    Serial.println("Setup done");
}

void loop(){
  lv_timer_handler();
  delay(5);
}
