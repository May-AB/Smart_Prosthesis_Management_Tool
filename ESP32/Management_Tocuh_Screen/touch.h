#ifndef TOUCH_H
#define TOUCH_H
/*******************************************************************************
 * Touch libraries:

 * GT911: https://github.com/TAMCTec/gt911-arduino.git

 ******************************************************************************/


/* uncomment for GT911 */
 #define TOUCH_GT911
 #define TOUCH_GT911_SCL 32
 #define TOUCH_GT911_SDA 33
 #define TOUCH_GT911_INT -1
 #define TOUCH_GT911_RST 25
 #define TOUCH_GT911_ROTATION ROTATION_RIGHT//ROTATION_NORMAL
 #define TOUCH_MAP_X1 320
 #define TOUCH_MAP_X2 0
 #define TOUCH_MAP_Y1 240
 #define TOUCH_MAP_Y2 0

int touchLastX = 0, touchLastY = 0;

#if defined(TOUCH_FT6X36)
#include <Wire.h>
#include <FT6X36.h>
FT6X36 ts(&Wire, TOUCH_FT6X36_INT);
bool touch_touched_flag = true, touch_released_flag = true;

#elif defined(TOUCH_GT911)
#include <Wire.h>
#include <Touch_GT911.h>
// #include <TAMC_GT911.h>
Touch_GT911 ts = Touch_GT911(
    TOUCH_GT911_SDA,
    TOUCH_GT911_SCL,
    TOUCH_GT911_INT,
    TOUCH_GT911_RST,
    max(TOUCH_MAP_X1, TOUCH_MAP_X2),
    max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

#elif defined(TOUCH_XPT2046)
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
XPT2046_Touchscreen ts(TOUCH_XPT2046_CS, TOUCH_XPT2046_INT);
#endif

#if defined(TOUCH_FT6X36)
void touch(TPoint p, TEvent e)
{
  if (e != TEvent::Tap && e != TEvent::DragStart && e != TEvent::DragMove && e != TEvent::DragEnd)
  {
    return;
  }
  // translation logic depends on screen rotation
#if defined(TOUCH_SWAP_XY)
  touchLastX = map(p.y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touchLastY = map(p.x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#else
  touchLastX = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touchLastY = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#endif
  switch (e)
  {
  case TEvent::Tap:
    Serial.println("Tap");
    touch_touched_flag = true;
    touch_released_flag = true;
    break;
  case TEvent::DragStart:
    Serial.println("DragStart");
    touch_touched_flag = true;
    break;
  case TEvent::DragMove:
    Serial.println("DragMove");
    touch_touched_flag = true;
    break;
  case TEvent::DragEnd:
    Serial.println("DragEnd");
    touch_released_flag = true;
    break;
  default:
    Serial.println("UNKNOWN");
    break;
  }
}
#endif

void touchInit()
{
#if defined(TOUCH_FT6X36)
  Wire.begin(TOUCH_FT6X36_SDA, TOUCH_FT6X36_SCL);
  ts.begin();
  ts.registerTouchHandler(touch);
 // ts.setRotation(TOUCH_FT6X36_ROTATION);

#elif defined(TOUCH_GT911)
  Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
  ts.begin();
  ts.setRotation(TOUCH_GT911_ROTATION);

#elif defined(TOUCH_XPT2046)
  SPI.begin(TOUCH_XPT2046_SCK, TOUCH_XPT2046_MISO, TOUCH_XPT2046_MOSI, TOUCH_XPT2046_CS);
  ts.begin();
  ts.setRotation(TOUCH_XPT2046_ROTATION);

#endif
}

bool touchHasSignal()
{
#if defined(TOUCH_FT6X36)
  ts.loop();
  return touch_touched_flag || touch_released_flag;

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  return ts.tirqTouched();

#else
  return false;
#endif
}

bool touchTouched()
{
#if defined(TOUCH_FT6X36)
  if (touch_touched_flag)
  {
    touch_touched_flag = false;
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_GT911)
  ts.read();
  if (ts.isTouched)
  {
#if defined(TOUCH_SWAP_XY)
    touchLastX = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touchLastY = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#else
    touchLastX = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touchLastY = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#endif
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_XPT2046)
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
#if defined(TOUCH_SWAP_XY)
    touchLastX = map(p.y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touchLastY = map(p.x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#else
    touchLastX = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touchLastY = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#endif
    return true;
  }
  else
  {
    return false;
  }

#else
  return false;
#endif
}

bool touchReleased()
{
#if defined(TOUCH_FT6X36)
  if (touch_released_flag)
  {
    touch_released_flag = false;
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  return true;

#else
  return false;
#endif
}

#endif //TOUCH_H
