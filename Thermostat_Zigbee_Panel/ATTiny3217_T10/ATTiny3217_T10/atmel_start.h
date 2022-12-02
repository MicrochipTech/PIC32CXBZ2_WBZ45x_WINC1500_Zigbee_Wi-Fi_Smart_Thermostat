#ifndef ATMEL_START_H_INCLUDED
#define ATMEL_START_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "include/driver_init.h"
#include "include/atmel_start_pins.h"

#include "touch.h"

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void);
void factory_reset(void);
void oled_display(void);
void touch_detected(int i);
void wTouch_button_read(void);

#ifdef __cplusplus
}
#endif
#endif
