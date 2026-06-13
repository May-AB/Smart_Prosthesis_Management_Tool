#ifndef UI_NAVIGATION_H
#define UI_NAVIGATION_H

#include <lvgl.h>

void setupWelcomeScreen();
void readYamlFromProtScreenFunction();
void searchClientBLEScreen();
void setupInitialUserScreen();
void setupMainUI();

void changeToMainScreen(lv_event_t *e);
void returnToMain(lv_event_t *e);
void loadYamlStep(lv_event_t *e);

void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area,
                 lv_color_t *color_p);
void myTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void showPasswordScreen(lv_event_t *e);
void IRAM_ATTR buttonPress(); // Emergency button ISR

#endif // UI_NAVIGATION_H
