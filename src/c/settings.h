#pragma once

#include <pebble.h>
#include <stdbool.h>

typedef enum {
  TIME_FMT_SYSTEM = 0,
  TIME_FMT_12H,
  TIME_FMT_24H
} TimeFormat;

typedef enum {
  RAIN_OFF = 0,
  RAIN_LIGHT,
  RAIN_MEDIUM,
  RAIN_HEAVY
} RainIntensity;

typedef enum {
  FADE_SLOW = 0,
  FADE_NORMAL,
  FADE_FAST
} FadeSpeed;

typedef enum {
  SEASON_AUTO = 0,
  SEASON_SPRING,
  SEASON_SUMMER,
  SEASON_AUTUMN,
  SEASON_WINTER
} SeasonMode;

typedef enum {
  TOD_AUTO = 0,
  TOD_DAWN,
  TOD_DAY,
  TOD_DUSK,
  TOD_NIGHT
} TimeOfDayMode;

typedef struct {
  TimeFormat time_format;
  bool leading_zero;
  bool show_date;
  bool show_season;
  RainIntensity rain_intensity;
  FadeSpeed fade_speed;
  SeasonMode season_mode;
  TimeOfDayMode tod_mode;
} FaceSettings;

typedef void (*SettingsChangedFn)(void);

void settings_init(void);
void settings_deinit(void);
void settings_set_changed_callback(SettingsChangedFn cb);
const FaceSettings *settings_get(void);
