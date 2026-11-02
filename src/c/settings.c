#include "settings.h"

enum {
  PERSIST_TIME_FORMAT = 1,
  PERSIST_LEADING_ZERO = 2,
  PERSIST_SHOW_DATE = 3,
  PERSIST_SHOW_SEASON = 4,
  PERSIST_RAIN = 5,
  PERSIST_FADE = 6,
  PERSIST_SEASON_MODE = 7,
  PERSIST_TOD_MODE = 8,
};

static FaceSettings s_settings;
static SettingsChangedFn s_changed;
static bool s_inbox_open;

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void settings_set_defaults(void) {
  s_settings.time_format = TIME_FMT_SYSTEM;
  s_settings.leading_zero = true;
  s_settings.show_date = false;
  s_settings.show_season = false;
  s_settings.rain_intensity = RAIN_MEDIUM;
  s_settings.fade_speed = FADE_NORMAL;
  s_settings.season_mode = SEASON_AUTO;
  s_settings.tod_mode = TOD_AUTO;
}

static void settings_load(void) {
  settings_set_defaults();
  if (persist_exists(PERSIST_TIME_FORMAT)) {
    s_settings.time_format = (TimeFormat)clamp_int(
        persist_read_int(PERSIST_TIME_FORMAT), TIME_FMT_SYSTEM, TIME_FMT_24H);
  }
  if (persist_exists(PERSIST_LEADING_ZERO)) {
    s_settings.leading_zero = persist_read_bool(PERSIST_LEADING_ZERO);
  }
  if (persist_exists(PERSIST_SHOW_DATE)) {
    s_settings.show_date = persist_read_bool(PERSIST_SHOW_DATE);
  }
  if (persist_exists(PERSIST_SHOW_SEASON)) {
    s_settings.show_season = persist_read_bool(PERSIST_SHOW_SEASON);
  }
  if (persist_exists(PERSIST_RAIN)) {
    s_settings.rain_intensity = (RainIntensity)clamp_int(
        persist_read_int(PERSIST_RAIN), RAIN_OFF, RAIN_HEAVY);
  }
  if (persist_exists(PERSIST_FADE)) {
    s_settings.fade_speed = (FadeSpeed)clamp_int(
        persist_read_int(PERSIST_FADE), FADE_SLOW, FADE_FAST);
  }
  if (persist_exists(PERSIST_SEASON_MODE)) {
    s_settings.season_mode = (SeasonMode)clamp_int(
        persist_read_int(PERSIST_SEASON_MODE), SEASON_AUTO, SEASON_WINTER);
  }
  if (persist_exists(PERSIST_TOD_MODE)) {
    s_settings.tod_mode = (TimeOfDayMode)clamp_int(
        persist_read_int(PERSIST_TOD_MODE), TOD_AUTO, TOD_NIGHT);
  }
}

static void settings_save(void) {
  persist_write_int(PERSIST_TIME_FORMAT, (int)s_settings.time_format);
  persist_write_bool(PERSIST_LEADING_ZERO, s_settings.leading_zero);
  persist_write_bool(PERSIST_SHOW_DATE, s_settings.show_date);
  persist_write_bool(PERSIST_SHOW_SEASON, s_settings.show_season);
  persist_write_int(PERSIST_RAIN, (int)s_settings.rain_intensity);
  persist_write_int(PERSIST_FADE, (int)s_settings.fade_speed);
  persist_write_int(PERSIST_SEASON_MODE, (int)s_settings.season_mode);
  persist_write_int(PERSIST_TOD_MODE, (int)s_settings.tod_mode);
}

static int tuple_as_int(Tuple *t, int fallback) {
  if (!t) return fallback;
  if (t->type == TUPLE_INT) return (int)t->value->int32;
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return fallback;
}

static bool tuple_as_bool(Tuple *t, bool fallback) {
  if (!t) return fallback;
  if (t->type == TUPLE_INT) return t->value->int32 != 0;
  if (t->type == TUPLE_CSTRING) {
    return strcmp(t->value->cstring, "true") == 0 ||
           strcmp(t->value->cstring, "1") == 0;
  }
  return fallback;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_TimeFormat);
  if (t) {
    s_settings.time_format = (TimeFormat)clamp_int(
        tuple_as_int(t, (int)s_settings.time_format), TIME_FMT_SYSTEM, TIME_FMT_24H);
  }
  t = dict_find(iter, MESSAGE_KEY_LeadingZero);
  if (t) {
    s_settings.leading_zero = tuple_as_bool(t, s_settings.leading_zero);
  }
  t = dict_find(iter, MESSAGE_KEY_ShowDate);
  if (t) {
    s_settings.show_date = tuple_as_bool(t, s_settings.show_date);
  }
  t = dict_find(iter, MESSAGE_KEY_ShowSeason);
  if (t) {
    s_settings.show_season = tuple_as_bool(t, s_settings.show_season);
  }
  t = dict_find(iter, MESSAGE_KEY_RainIntensity);
  if (t) {
    s_settings.rain_intensity = (RainIntensity)clamp_int(
        tuple_as_int(t, (int)s_settings.rain_intensity), RAIN_OFF, RAIN_HEAVY);
  }
  t = dict_find(iter, MESSAGE_KEY_FadeSpeed);
  if (t) {
    s_settings.fade_speed = (FadeSpeed)clamp_int(
        tuple_as_int(t, (int)s_settings.fade_speed), FADE_SLOW, FADE_FAST);
  }
  t = dict_find(iter, MESSAGE_KEY_SeasonMode);
  if (t) {
    s_settings.season_mode = (SeasonMode)clamp_int(
        tuple_as_int(t, (int)s_settings.season_mode), SEASON_AUTO, SEASON_WINTER);
  }
  t = dict_find(iter, MESSAGE_KEY_TimeOfDayMode);
  if (t) {
    s_settings.tod_mode = (TimeOfDayMode)clamp_int(
        tuple_as_int(t, (int)s_settings.tod_mode), TOD_AUTO, TOD_NIGHT);
  }

  settings_save();
  if (s_changed) {
    s_changed();
  }
}

void settings_init(void) {
  settings_load();
  if (!s_inbox_open) {
    app_message_register_inbox_received(inbox_received);
    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());
    s_inbox_open = true;
  }
}

void settings_deinit(void) {
  settings_save();
  if (s_inbox_open) {
    app_message_deregister_callbacks();
    s_inbox_open = false;
  }
}

void settings_set_changed_callback(SettingsChangedFn cb) {
  s_changed = cb;
}

const FaceSettings *settings_get(void) {
  return &s_settings;
}
