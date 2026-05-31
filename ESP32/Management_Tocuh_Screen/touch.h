#ifndef TOUCH_H
#define TOUCH_H
/*******************************************************************************
 *This code is based on Arduino_GFX library examples.
 
 * Touch libraries:

 * GT911: https://github.com/TAMCTec/gt911-arduino.git

 ******************************************************************************/

#include <Wire.h>
#include <Touch_GT911.h>

extern int touchLastX;
extern int touchLastY;
extern Touch_GT911 ts;

void touchInit();
bool touchHasSignal();
bool touchTouched();
bool touchReleased();

#endif //TOUCH_H
