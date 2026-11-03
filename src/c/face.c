#include "face.h"
#include "atmosphere.h"
#include "rain.h"
#include "settings.h"

static Window *s_window;
static Layer *s_layer;

static GBitmap *s_outside;
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
  (void)art;
  // Full-bleed: the whole display is the windowpane (scale art to fill).
  return GRect(0, 0, bounds.w, bounds.h);
}

// Deterministic hash for stable mist / dither speckles.
static uint16_t hash16(int x, int y, int salt) {
  uint16_t h = (uint16_t)(x * 3741 + y * 1237 + salt * 97 + s_minute * 17);
  h ^= (uint16_t)(h << 7);
  h ^= (uint16_t)(h >> 9);
  h ^= (uint16_t)(h << 8);
  return h;
}

static void draw_tod_dither(GContext *ctx, GRect pane, TodId tod) {
#if defined(PBL_COLOR)
  // Sparse pixel wash — never a solid fill (those made the bullseye).
  GColor tint;
  int chance;  // higher = denser
  switch (tod) {
    case TOD_DAWN_ID:
      tint = GColorMelon;
      chance = 5;  // ~1/5 of edge/sky pixels
      break;
    case TOD_DUSK_ID:
      tint = GColorSunsetOrange;
      chance = 4;
      break;
    case TOD_NIGHT_ID:
      tint = GColorBlack;
      chance = 2;
      break;
    case TOD_DAY_ID:
    default:
      return;
  }

  graphics_context_set_stroke_color(ctx, tint);
  const int band = pane.size.h / 3;
  // Step by 2 — enough sparkle without scanning every pixel each frame.
  for (int y = pane.origin.y; y < pane.origin.y + pane.size.h; y += 2) {
    for (int x = pane.origin.x; x < pane.origin.x + pane.size.w; x += 2) {
      int in_sky = (y < pane.origin.y + band);
      int edge =
          (x < pane.origin.x + 10) || (x > pane.origin.x + pane.size.w - 11) ||
          (y < pane.origin.y + 8) || (y > pane.origin.y + pane.size.h - 9);
      int dens = in_sky ? chance : (tod == TOD_NIGHT_ID ? chance + 1 : chance + 3);
      if (!in_sky && !edge && tod != TOD_NIGHT_ID) continue;
      if ((hash16(x, y, (int)tod) % dens) == 0) {
        graphics_draw_pixel(ctx, GPoint(x, y));
      }
    }
  }
#else
  if (tod == TOD_NIGHT_ID) {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    for (int y = pane.origin.y; y < pane.origin.y + pane.size.h; y += 2) {
      for (int x = pane.origin.x + (y & 2); x < pane.origin.x + pane.size.w; x += 2) {
        graphics_draw_pixel(ctx, GPoint(x, y));
      }
    }
  }
#endif
}

static void draw_edge_mist(GContext *ctx, GRect pane, int fog_extra) {
  // Condensation gathers at the frame edges — speckles, not a circular wipe.
#if defined(PBL_COLOR)
  graphics_context_set_stroke_color(ctx, GColorLightGray);
#else
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  const int margin = 18 + fog_extra;
  for (int y = pane.origin.y; y < pane.origin.y + pane.size.h; y += 2) {
    for (int x = pane.origin.x; x < pane.origin.x + pane.size.w; x += 2) {
      int dx = x - pane.origin.x;
      int dy = y - pane.origin.y;
      int dxr = pane.size.w - 1 - dx;
      int dyr = pane.size.h - 1 - dy;
      int edge = dx;
      if (dxr < edge) edge = dxr;
      if (dy < edge) edge = dy;
      if (dyr < edge) edge = dyr;
      if (edge > margin) continue;
      // Denser nearer the frame.
      int thresh = 2 + (edge * 6) / (margin + 1);
      if ((hash16(x, y, 3) % (thresh + 1)) == 0) {
        graphics_draw_pixel(ctx, GPoint(x, y));
      }
    }
  }

  // A few vertical drip streaks on the glass.
#if defined(PBL_COLOR)
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
#else
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  for (int i = 0; i < 7; i++) {
    int x = pane.origin.x + 12 + (int)(hash16(i, 9, s_minute) % (pane.size.w > 24 ? pane.size.w - 24 : 1));
    int y0 = pane.origin.y + 6 + (int)(hash16(i, 2, 1) % 20);
    int len = 18 + (int)(hash16(i, 4, 2) % 40);
    graphics_draw_line(ctx, GPoint(x, y0), GPoint(x, y0 + len));
  }
}

static void draw_window_frame(GContext *ctx, GRect pane) {
  // Thin edge trim only — screen itself is the glass, not an inset picture.
#if defined(PBL_COLOR)
  graphics_context_set_stroke_color(ctx, GColorBlack);
#else
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_rect(ctx, pane);
  graphics_context_set_stroke_width(ctx, 1);
}

static void draw_condensation_time(GContext *ctx, GRect bounds, GRect pane,
                                   TodId tod, int fade) {
  char time_buf[16];
  format_time(time_buf, sizeof(time_buf));

  const int time_y = pane.origin.y + pane.size.h / 2 - 28;
  GFont font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  GRect time_rect = GRect(0, time_y, bounds.size.w, 44);

  // Fresh wipe: bright digits with a soft fog "halo" behind them (cleared glass).
  // As fade rises, digits sink into mist and drip trails dry up.
  if (fade <= 4) {
#if defined(PBL_COLOR)
    graphics_context_set_stroke_color(ctx, GColorLightGray);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
    // Horizontal smear / wipe mark under the numerals.
    int smear_y = time_y + 38;
    int half = 36 - fade * 4;
    if (half < 8) half = 8;
    graphics_draw_line(ctx,
                       GPoint(bounds.size.w / 2 - half, smear_y),
                       GPoint(bounds.size.w / 2 + half, smear_y));
    if (fade <= 2) {
      graphics_draw_line(ctx,
                         GPoint(bounds.size.w / 2 - half + 4, smear_y + 2),
                         GPoint(bounds.size.w / 2 + half - 4, smear_y + 2));
    }

    // Condensation drips falling from the wiped digits.
    if (fade <= 3) {
      for (int i = 0; i < 5 - fade; i++) {
        int dx = -28 + i * 14;
        int x = bounds.size.w / 2 + dx;
        int y1 = smear_y + 3;
        int y2 = y1 + 6 + (3 - fade) * 3 + (int)(hash16(i, s_minute, 5) % 5);
        graphics_draw_line(ctx, GPoint(x, y1), GPoint(x, y2));
        graphics_draw_pixel(ctx, GPoint(x, y2 + 1));
      }
    }
  }

  // Shadow pass (wet glass depth), then the condensation color.
  if (fade <= 5) {
#if defined(PBL_COLOR)
    graphics_context_set_text_color(ctx, GColorDarkGray);
#else
    graphics_context_set_text_color(ctx, GColorBlack);
#endif
    graphics_draw_text(ctx, time_buf, font,
                       GRect(time_rect.origin.x + 1, time_rect.origin.y + 1,
                             time_rect.size.w, time_rect.size.h),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  GColor time_color = atmosphere_condensation_color(fade, tod);
  // Late fade: only draw every other "ghost" frame via dither skip.
  if (fade >= 6) {
    // Almost gone — speckled ghost of the time.
#if defined(PBL_COLOR)
    graphics_context_set_text_color(ctx, GColorDarkGray);
#else
    graphics_context_set_text_color(ctx, GColorWhite);
#endif
    if ((s_second & 1) == 0) {
      graphics_draw_text(ctx, time_buf, font, time_rect,
                         GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentCenter, NULL);
    }
  } else {
    graphics_context_set_text_color(ctx, time_color);
    graphics_draw_text(ctx, time_buf, font, time_rect,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  if (settings_get()->show_date) {
    char date_buf[24];
    format_date(date_buf, sizeof(date_buf));
    graphics_context_set_text_color(ctx, time_color);
    graphics_draw_text(ctx, date_buf,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, time_y + 44, bounds.size.w, 18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }
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
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, s_outside, pane);
    draw_tod_dither(ctx, pane, tod);
  }

  int fade = condensation_fade_level();
  // Mist thickens as the wiped time fogs back over.
  draw_edge_mist(ctx, pane, fade / 2);

  draw_window_frame(ctx, pane);

  if (cfg->rain_intensity != RAIN_OFF) {
    rain_draw(ctx, bounds.size, NULL);  // streak rain only — no chunky sprites
  }

  draw_condensation_time(ctx, bounds, pane, tod, fade);

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
  unload_outside();
  face_tick_minute();
}
