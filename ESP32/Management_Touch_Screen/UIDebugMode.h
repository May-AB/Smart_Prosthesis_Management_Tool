#ifndef UI_DEBUG_MODE_H
#define UI_DEBUG_MODE_H

#include <lvgl.h>

void createDebugModeControls(lv_obj_t *parent);
void deleteDebug();
// Called from BLE core to safely update the chart on the UI core
void scheduleChartUpdate(int hardwareValue);

#endif // UI_DEBUG_MODE_H
