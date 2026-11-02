#include "rain.h"

#define MAX_DROPS 24

typedef struct {
  int16_t x;
  int16_t y;
  int8_t speed;
  int8_t len;
  bool active;
} Drop;

static Drop s_drops[MAX_DROPS];
static int s_active_count;
static uint16_t s_rng;

static uint16_t rng_next(void) {
  // xorshift16
  s_rng ^= (uint16_t)(s_rng << 7);
  s_rng ^= (uint16_t)(s_rng >> 9);
  s_rng ^= (uint16_t)(s_rng << 8);
  if (s_rng == 0) s_rng = 0xA5A5;
  return s_rng;
}

static int drop_budget(RainIntensity intensity) {
  switch (intensity) {
    case RAIN_OFF: return 0;
    case RAIN_LIGHT: return 8;
    case RAIN_MEDIUM: return 14;
    case RAIN_HEAVY: return MAX_DROPS;
    default: return 14;
  }
}

static void spawn_drop(Drop *d, GSize bounds, bool anywhere) {
  d->x = (int16_t)(rng_next() % (uint16_t)(bounds.w > 2 ? bounds.w - 2 : 1));
  d->y = anywhere ? (int16_t)(rng_next() % (uint16_t)(bounds.h > 1 ? bounds.h : 1))
                  : (int16_t)(-(rng_next() % 24));
  d->speed = (int8_t)(1 + (rng_next() % 3));
  d->len = (int8_t)(3 + (rng_next() % 6));
  d->active = true;
}

void rain_init(void) {
  time_t now = time(NULL);
  s_rng = (uint16_t)(now ^ (now >> 16) ^ 0xBEEF);
  if (s_rng == 0) s_rng = 1;
  s_active_count = 0;
  for (int i = 0; i < MAX_DROPS; i++) {
    s_drops[i].active = false;
  }
}

void rain_deinit(void) {
  s_active_count = 0;
}

void rain_reset(GSize bounds, RainIntensity intensity) {
  int budget = drop_budget(intensity);
  s_active_count = budget;
  for (int i = 0; i < MAX_DROPS; i++) {
    if (i < budget) {
      spawn_drop(&s_drops[i], bounds, true);
    } else {
      s_drops[i].active = false;
    }
  }
}

void rain_tick(GSize bounds) {
  for (int i = 0; i < s_active_count; i++) {
    Drop *d = &s_drops[i];
    if (!d->active) {
      spawn_drop(d, bounds, false);
      continue;
    }
    d->y = (int16_t)(d->y + d->speed);
    // Slight horizontal drift as drops slide on glass.
    if ((rng_next() & 3) == 0) {
      d->x = (int16_t)(d->x + ((rng_next() & 1) ? 1 : -1));
      if (d->x < 0) d->x = 0;
      if (d->x >= bounds.w) d->x = (int16_t)(bounds.w - 1);
    }
    if (d->y > bounds.h + 4) {
      spawn_drop(d, bounds, false);
    }
  }
}

void rain_draw(GContext *ctx, GSize bounds, GBitmap *drop_sprite) {
  (void)bounds;
#if defined(PBL_COLOR)
  graphics_context_set_stroke_color(ctx, GColorPictonBlue);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif

  for (int i = 0; i < s_active_count; i++) {
    Drop *d = &s_drops[i];
    if (!d->active) continue;

    if (drop_sprite && (i & 1)) {
      GSize sz = gbitmap_get_bounds(drop_sprite).size;
      GRect dest = GRect(d->x - sz.w / 2, d->y, sz.w, sz.h);
      graphics_draw_bitmap_in_rect(ctx, drop_sprite, dest);
    } else {
      graphics_draw_line(ctx, GPoint(d->x, d->y), GPoint(d->x, d->y + d->len));
      if (d->len > 4) {
        graphics_draw_pixel(ctx, GPoint(d->x, d->y + d->len + 1));
      }
    }
  }
}
