#include <Arduino.h>
#include <Wire.h>
#include <Touch_GT911.h>
#include "ConfigParams.h"
#include "Touch.h"

int touchLastX = 0;
int touchLastY = 0;

Touch_GT911 ts = Touch_GT911(
    TOUCH_GT911_SDA,
    TOUCH_GT911_SCL,
    TOUCH_GT911_INT,
    TOUCH_GT911_RST,
    max(TOUCH_MAP_X1, TOUCH_MAP_X2),
    max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

void touchInit(){
  Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
  ts.begin();
  ts.setRotation(TOUCH_GT911_ROTATION);
}

bool touchHasSignal()
{
  return true;
}

bool touchTouched(){
  ts.read();
  if (ts.isTouched) {
  #if defined(TOUCH_SWAP_XY)
      touchLastX = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
      touchLastY = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
  #else
      touchLastX = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
      touchLastY = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
  #endif
      return true;
  }
  else  {
    return false;
  }
}

bool touchReleased() {
  return true;
}
