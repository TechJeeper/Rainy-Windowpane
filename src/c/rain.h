#pragma once

#include <pebble.h>
#include "settings.h"

void rain_init(void);
void rain_deinit(void);
void rain_reset(GSize bounds, RainIntensity intensity);
void rain_tick(GSize bounds);
void rain_draw(GContext *ctx, GSize bounds, GBitmap *drop_sprite);
