#include <pebble.h>
#include "face.h"
#include "settings.h"

static Window *s_window;
static AppTimer *s_anim_timer;

static void on_tick(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;
  face_tick_minute();
}

static void anim_cb(void *ctx) {
  (void)ctx;
  s_anim_timer = app_timer_register(200, anim_cb, NULL);
  face_tick_anim();
}

static void on_settings_changed(void) {
  face_apply_settings();
}

static void window_load(Window *window) {
  face_init(window);
}

static void window_unload(Window *window) {
  (void)window;
  face_deinit();
}

static void init(void) {
  settings_init();
  settings_set_changed_callback(on_settings_changed);

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, on_tick);
  s_anim_timer = app_timer_register(200, anim_cb, NULL);
}

static void deinit(void) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
  tick_timer_service_unsubscribe();
  settings_deinit();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
