#pragma once

#include <pebble.h>

void face_init(Window *window);
void face_deinit(void);
void face_tick_minute(void);
void face_tick_anim(void);
void face_apply_settings(void);
