#include "face.h"
#include "atmosphere.h"
#include "rain.h"
#include "settings.h"

static Window *s_window;
static Layer *s_layer;

static GBitmap *s_outside;
static GBitmap *s_fog;
static GBitmap *s_drop;
static SeasonId s_loaded_season = (SeasonId)-1;

static int s_hour;
static int s_minute;
static int s_second;
static int s_wday;
static int s_mday;
static int s_mon;

static void unload_outside(void) {
  if (s_outside) {
    gbitmap_destroy(s_outside);
    s_outside = NULL;
  }
  s_loaded_season = (SeasonId)-1;
}

static void load_outside(SeasonId season) {
  if (s_loaded_season == season && s_outside) {
    return;
  }
  unload_outside();
  s_outside = gbitmap_create_with_resource(atmosphere_outside_resource(season));
  s_loaded_season = season;
}

static bool use_24h(void) {
  const FaceSettings *s = settings_get();
  if (s->time_format == TIME_FMT_24H) return true;
  if (s->time_format == TIME_FMT_12H) return false;
  return clock_is_24h_style();
}

static void format_time(char *buf, size_t n) {
  int hour = s_hour;
  const bool is24 = use_24h();
  if (!is24) {
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }
  if (settings_get()->leading_zero) {
    snprintf(buf, n, "%02d:%02d", hour, s_minute);
  } else {
    snprintf(buf, n, "%d:%02d", hour, s_minute);
  }
}

static void format_date(char *buf, size_t n) {
  static const char *const DOW[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char *const MON[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  snprintf(buf, n, "%s %d %s", DOW[s_wday % 7], s_mday, MON[s_mon % 12]);
}

static int condensation_fade_level(void) {
  const FaceSettings *cfg = settings_get();
  // Map seconds into 0..7. Faster fade reaches "gone" sooner, then stays faint
  // until the next minute redraws a fresh wipe.
  int span;
  switch (cfg->fade_speed) {
    case FADE_SLOW: span = 55; break;
    case FADE_FAST: span = 25; break;
    case FADE_NORMAL:
    default: span = 40; break;
  }
  int level = (s_second * 7) / span;
  if (level > 7) level = 7;
  return level;
}

static GRect outside_dest(GSize bounds, GSize art) {
  // Center artwork; on larger panels (Emery) it sits as a window pane.
  int x = (bounds.w - art.w) / 2;
  int y = (bounds.h - art.h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  return GRect(x, y, art.w, art.h);
}

static void draw_tod_wash(GContext *ctx, GRect pane, TodId tod) {
#if defined(PBL_COLOR)
  // Sky band picks up dawn / day / dusk / night light.
  GColor sky = atmosphere_sky_tint(tod);
  graphics_context_set_fill_color(ctx, sky);
  graphics_context_set_compositing_mode(ctx, GCompOpAnd);
  int band = pane.size.h / 3;
  graphics_fill_rect(ctx, GRect(pane.origin.x, pane.origin.y, pane.size.w, band),
                     0, GCornerNone);

  if (tod == TOD_NIGHT_ID || tod == TOD_DUSK_ID) {
    graphics_context_set_fill_color(ctx, atmosphere_night_wash(tod));
    graphics_context_set_compositing_mode(ctx, GCompOpAnd);
    graphics_fill_rect(ctx, pane, 0, GCornerNone);
  } else if (tod == TOD_DAWN_ID) {
    // Warm lower glow at dawn.
    graphics_context_set_fill_color(ctx, GColorMelon);
    graphics_context_set_compositing_mode(ctx, GCompOpOr);
    graphics_fill_rect(ctx,
                       GRect(pane.origin.x, pane.origin.y + pane.size.h - band,
                             pane.size.w, band),
                       0, GCornerNone);
  }
#else
  // B&W: night inverts the feel with a dark top band dither.
  if (tod == TOD_NIGHT_ID) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(pane.origin.x, pane.origin.y, pane.size.w, pane.size.h / 3),
                       0, GCornerNone);
  }
  (void)ctx;
#endif
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void draw_window_frame(GContext *ctx, GRect pane) {
#if defined(PBL_COLOR)
  graphics_context_set_stroke_color(ctx, GColorBlack);
#else
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_rect(ctx, pane);
  graphics_context_set_stroke_width(ctx, 1);
  // Mullion cross.
  int mx = pane.origin.x + pane.size.w / 2;
  int my = pane.origin.y + pane.size.h / 2;
  graphics_draw_line(ctx, GPoint(mx, pane.origin.y), GPoint(mx, pane.origin.y + pane.size.h));
  graphics_draw_line(ctx, GPoint(pane.origin.x, my), GPoint(pane.origin.x + pane.size.w, my));
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const FaceSettings *cfg = settings_get();

  SeasonId season = atmosphere_season(s_mon, cfg->season_mode);
  TodId tod = atmosphere_tod(s_hour, cfg->tod_mode);
  load_outside(season);

  GColor bg = atmosphere_window_bg(tod);
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GRect pane = bounds;
  if (s_outside) {
    GSize art = gbitmap_get_bounds(s_outside).size;
    pane = outside_dest(bounds.size, art);
#if defined(PBL_COLOR)
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#else
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
    graphics_draw_bitmap_in_rect(ctx, s_outside, pane);
    draw_tod_wash(ctx, pane, tod);
  }

  if (s_fog) {
#if defined(PBL_COLOR)
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
    graphics_context_set_compositing_mode(ctx, GCompOpOr);
#endif
    graphics_draw_bitmap_in_rect(ctx, s_fog, pane);
  }

  draw_window_frame(ctx, pane);

  // Rain slides on the glass.
  if (cfg->rain_intensity != RAIN_OFF) {
    rain_draw(ctx, bounds.size, s_drop);
  }

  // Condensation time — freshly wiped at :00, then fogs over.
  char time_buf[16];
  format_time(time_buf, sizeof(time_buf));
  int fade = condensation_fade_level();
  GColor time_color = atmosphere_condensation_color(fade, tod);
  graphics_context_set_text_color(ctx, time_color);

  const int time_y = pane.origin.y + pane.size.h / 2 - 28;
  graphics_draw_text(ctx, time_buf,
                     fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                     GRect(0, time_y, bounds.size.w, 44),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Soft "smear" under the digits while still fresh.
  if (fade <= 2) {
#if defined(PBL_COLOR)
    graphics_context_set_stroke_color(ctx, GColorLightGray);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
    int smear_y = time_y + 40;
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2 - 40, smear_y),
                       GPoint(bounds.size.w / 2 + 40, smear_y));
  }

  if (cfg->show_date) {
    char date_buf[24];
    format_date(date_buf, sizeof(date_buf));
    graphics_context_set_text_color(ctx, time_color);
    graphics_draw_text(ctx, date_buf,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, time_y + 44, bounds.size.w, 18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  if (cfg->show_season) {
#if defined(PBL_COLOR)
    graphics_context_set_text_color(ctx, GColorLightGray);
#else
    graphics_context_set_text_color(ctx, GColorWhite);
#endif
    graphics_draw_text(ctx, atmosphere_season_label(season),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, bounds.size.h - 20, bounds.size.w, 18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }
}

static void mark_dirty(void) {
  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
  if (s_window) {
    TodId tod = atmosphere_tod(s_hour, settings_get()->tod_mode);
    window_set_background_color(s_window, atmosphere_window_bg(tod));
  }
}

void face_init(Window *window) {
  s_window = window;

  s_fog = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FOG_OVERLAY);
  s_drop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RAINDROP);

  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(root, s_layer);

  rain_init();
  rain_reset(bounds.size, settings_get()->rain_intensity);

  face_tick_minute();
  mark_dirty();
}

void face_deinit(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
  unload_outside();
  if (s_fog) {
    gbitmap_destroy(s_fog);
    s_fog = NULL;
  }
  if (s_drop) {
    gbitmap_destroy(s_drop);
    s_drop = NULL;
  }
  rain_deinit();
  s_window = NULL;
}

void face_tick_minute(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_minute = t->tm_min;
  s_second = t->tm_sec;
  s_wday = t->tm_wday;
  s_mday = t->tm_mday;
  s_mon = t->tm_mon;
  mark_dirty();
}

void face_tick_anim(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_second = t->tm_sec;
  // Keep hour/season in sync if the anim timer spans a boundary.
  s_hour = t->tm_hour;
  s_minute = t->tm_min;
  s_mon = t->tm_mon;

  if (s_layer) {
    GRect bounds = layer_get_bounds(s_layer);
    rain_tick(bounds.size);
  }
  mark_dirty();
}

void face_apply_settings(void) {
  if (s_layer) {
    rain_reset(layer_get_bounds(s_layer).size, settings_get()->rain_intensity);
  }
  // Force season bitmap reload on next draw.
  unload_outside();
  face_tick_minute();
}
