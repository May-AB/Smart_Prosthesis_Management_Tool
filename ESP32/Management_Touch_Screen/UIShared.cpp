#include "UIShared.h"
#include <Arduino.h>
#include <SharedYamlParser.h>

lv_obj_t *createNewBtn(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                       lv_align_t align, lv_coord_t xOfs, lv_coord_t yOfs,
                       const char *name, lv_color_t btnColor,
                       lv_color_t labelColor, const lv_font_t *labelSize) {
  lv_obj_t *new_btn = lv_btn_create(parent);
  lv_obj_set_size(new_btn, w, h);
  lv_obj_set_style_bg_color(new_btn, btnColor, 0);
  lv_obj_align(new_btn, align, xOfs, yOfs);

  lv_obj_t *label = lv_label_create(new_btn);
  lv_obj_set_style_text_color(label, labelColor, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_AUTO, 0);
  lv_obj_set_style_text_font(label, labelSize, 0);
  lv_label_set_text(label, name);

  return new_btn;
}

void designBtnm(lv_event_t *e) {
  lv_color_t *color = (lv_color_t *)lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS) {
    dsc->rect_dsc->bg_color = *color;
  }
}

void designLabelBig(lv_event_t *e) {
  lv_color_t *color = (lv_color_t *)lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS) {
    dsc->label_dsc->color = *color;
    dsc->label_dsc->font = &lv_font_montserrat_20;
  }
}

void designLabelSmall(lv_event_t *e) {
  lv_color_t *color = (lv_color_t *)lv_event_get_user_data(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part == LV_PART_ITEMS) {
    dsc->label_dsc->color = *color;
    dsc->label_dsc->font = &lv_font_montserrat_10;
  }
}

lv_obj_t *createNewMatrixBtnChooseOne(
    lv_obj_t *parent, char ***map, int lenMap, lv_align_t align,
    lv_coord_t xOfs, lv_coord_t yOfs, lv_color_t bgColor, lv_color_t btnsColor,
    lv_color_t labelsColor, bool big, lv_coord_t screenWidth,
    lv_coord_t screenHeight, int maxInRow, bool needToFreeOldMap) {
  lv_obj_t *btnm = lv_btnmatrix_create(parent);

  // fit labels to matrix
  if (maxInRow > 0) {
    // create modify map
    int newNumPointer = lenMap + (lenMap - 1) / maxInRow;
    char **tempPtr = (char **)malloc(newNumPointer * sizeof(char *));
    int j = 0;
    for (int i = 0; i < lenMap - 1; i++) {
      tempPtr[j] = *((*map) + i);
      if (i % maxInRow == (maxInRow - 1)) {
        j++;
        tempPtr[j] = "\n";
      }
      j++;
    }
    tempPtr[j] = "";

    if (needToFreeOldMap) {
      free(*map);
    }
    (*map) = tempPtr;
  }

  lv_btnmatrix_set_map(btnm, (const char **)(*map));
  lv_btnmatrix_set_one_checked(btnm, true);
  lv_obj_set_size(btnm, screenWidth * 0.7, screenHeight * 0.7);
  lv_obj_set_style_bg_color(btnm, bgColor, 0);

  // design each btn color
  // Allocate color on the heap to avoid static reuse bug (from Phase 3.3)
  lv_color_t *btnsColorPtr = (lv_color_t *)lv_mem_alloc(sizeof(lv_color_t));
  *btnsColorPtr = btnsColor;
  lv_obj_add_event_cb(btnm, designBtnm, LV_EVENT_DRAW_PART_BEGIN, btnsColorPtr);

  lv_color_t *labelsColorPtr = (lv_color_t *)lv_mem_alloc(sizeof(lv_color_t));
  *labelsColorPtr = labelsColor;
  if (big) {
    lv_obj_add_event_cb(btnm, designLabelBig, LV_EVENT_DRAW_PART_BEGIN,
                        labelsColorPtr);
  } else {
    lv_obj_add_event_cb(btnm, designLabelSmall, LV_EVENT_DRAW_PART_BEGIN,
                        labelsColorPtr);
  }

  // Free allocated memory when the matrix is deleted
  lv_obj_add_event_cb(
      btnm,
      [](lv_event_t *e) {
        void *userData = lv_event_get_user_data(e);
        if (userData) {
          lv_mem_free(userData);
        }
      },
      LV_EVENT_DELETE, btnsColorPtr);

  lv_obj_add_event_cb(
      btnm,
      [](lv_event_t *e) {
        void *userData = lv_event_get_user_data(e);
        if (userData) {
          lv_mem_free(userData);
        }
      },
      LV_EVENT_DELETE, labelsColorPtr);

  lv_obj_align(btnm, align, xOfs, yOfs);
  return btnm;
}

void getOptionsString(char *destBuffer, size_t destSize, bool isMotors) {
  if (destBuffer == nullptr || destSize == 0)
    return;
  destBuffer[0] = '\0';
  size_t ind = 0;
  if (isMotors) {
    for (const auto &motor : motors) {
      int written = snprintf(&destBuffer[ind], destSize - ind, "%s\n",
                             motor.name.c_str());
      if (written > 0 && ind + written < destSize) {
        ind += written;
      }
    }
  } else {
    for (const auto &sensor : sensors) {
      int written = snprintf(&destBuffer[ind], destSize - ind, "%s\n",
                             sensor.name.c_str());
      if (written > 0 && ind + written < destSize) {
        ind += written;
      }
    }
  }
  if (ind > 0) {
    destBuffer[ind - 1] = '\0'; // strip trailing newline
  }
}
