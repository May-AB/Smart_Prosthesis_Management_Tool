#ifndef UI_SHARED_H
#define UI_SHARED_H

#include "ConfigParams.h"
#include <lvgl.h>
#include <vector>

struct ReturnUnsavedParam {
  int itemId; // Renamed from sensorId to avoid confusion since it is used for
              // both motors and sensors
  std::vector<int> paramsId;
  std::vector<int> newVals;
};

// Shared UI element helper functions
lv_obj_t *createNewBtn(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                       lv_align_t align, lv_coord_t xOfs, lv_coord_t yOfs,
                       const char *name, lv_color_t btnColor,
                       lv_color_t labelColor,
                       const lv_font_t *labelSize = &lv_font_montserrat_14);

lv_obj_t *createNewMatrixBtnChooseOne(
    lv_obj_t *parent, char ***map, int lenMap, lv_align_t align,
    lv_coord_t xOfs, lv_coord_t yOfs, lv_color_t bgColor, lv_color_t btnsColor,
    lv_color_t labelsColor, bool big = true, lv_coord_t screenWidth = 320,
    lv_coord_t screenHeight = 240, int maxInRow = -1,
    bool needToFreeOldMap = false);

void designBtnm(lv_event_t *e);
void designLabelBig(lv_event_t *e);
void designLabelSmall(lv_event_t *e);
void getOptionsString(char *destBuffer, size_t destSize, bool isMotors);

#endif // UI_SHARED_H
