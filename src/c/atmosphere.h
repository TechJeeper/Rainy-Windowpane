#pragma once

#include <pebble.h>
#include "settings.h"

typedef enum {
  SEASON_SPRING_ID = 0,
  SEASON_SUMMER_ID,
  SEASON_AUTUMN_ID,
  SEASON_WINTER_ID,
  SEASON_COUNT
} SeasonId;

typedef enum {
  TOD_DAWN_ID = 0,
  TOD_DAY_ID,
  TOD_DUSK_ID,
  TOD_NIGHT_ID,
  TOD_COUNT
} TodId;

SeasonId atmosphere_season(int month /* 0-11 */, SeasonMode mode);
TodId atmosphere_tod(int hour /* 0-23 */, TimeOfDayMode mode);
const char *atmosphere_season_label(SeasonId season);
uint32_t atmosphere_outside_resource(SeasonId season);
GColor atmosphere_sky_tint(TodId tod);
GColor atmosphere_night_wash(TodId tod);
GColor atmosphere_condensation_color(int fade_level /* 0 fresh .. 7 gone */, TodId tod);
GColor atmosphere_window_bg(TodId tod);
