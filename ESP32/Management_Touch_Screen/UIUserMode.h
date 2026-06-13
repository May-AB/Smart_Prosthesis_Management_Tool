#ifndef UI_USER_MODE_H
#define UI_USER_MODE_H

#include <lvgl.h>
#include <vector>

void createControlsForMain(lv_obj_t *parent);
void createControlsForStat(lv_obj_t *parent);
void createControlsForSetup(lv_obj_t *parent);

std::vector<int> findNewOffSensor();
std::vector<int> findNewOnSensor();

#endif // UI_USER_MODE_H
