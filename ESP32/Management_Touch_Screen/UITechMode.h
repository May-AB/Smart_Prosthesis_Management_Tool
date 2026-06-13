#ifndef UI_TECH_MODE_H
#define UI_TECH_MODE_H

#include "UIShared.h"
#include <lvgl.h>

void createControlsForTech(lv_obj_t *parent);
ReturnUnsavedParam checkUnsaveSensorParam();
ReturnUnsavedParam checkUnsaveMotorParam();
// Resets edit-session state (called by UINavigation on screen transitions)
void clearEditState();

#endif // UI_TECH_MODE_H
