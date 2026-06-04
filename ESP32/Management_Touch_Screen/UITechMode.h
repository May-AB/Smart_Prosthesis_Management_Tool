#ifndef UI_TECH_MODE_H
#define UI_TECH_MODE_H

#include <lvgl.h>
#include "UIShared.h"

void createControlsForTech(lv_obj_t* parent);
ReturnUnsavedParam checkUnsaveSensorParam();
ReturnUnsavedParam checkUnsaveMotorParam();

#endif // UI_TECH_MODE_H
