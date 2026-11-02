#include "atmosphere.h"

SeasonId atmosphere_season(int month, SeasonMode mode) {
  if (mode == SEASON_SPRING) return SEASON_SPRING_ID;
  if (mode == SEASON_SUMMER) return SEASON_SUMMER_ID;
  if (mode == SEASON_AUTUMN) return SEASON_AUTUMN_ID;
  if (mode == SEASON_WINTER) return SEASON_WINTER_ID;

  // Meteorological seasons (Northern Hemisphere).
  if (month >= 2 && month <= 4) return SEASON_SPRING_ID;
  if (month >= 5 && month <= 7) return SEASON_SUMMER_ID;
  if (month >= 8 && month <= 10) return SEASON_AUTUMN_ID;
  return SEASON_WINTER_ID;
}

TodId atmosphere_tod(int hour, TimeOfDayMode mode) {
  if (mode == TOD_DAWN) return TOD_DAWN_ID;
  if (mode == TOD_DAY) return TOD_DAY_ID;
  if (mode == TOD_DUSK) return TOD_DUSK_ID;
  if (mode == TOD_NIGHT) return TOD_NIGHT_ID;

  if (hour >= 5 && hour < 8) return TOD_DAWN_ID;
  if (hour >= 8 && hour < 17) return TOD_DAY_ID;
  if (hour >= 17 && hour < 21) return TOD_DUSK_ID;
  return TOD_NIGHT_ID;
}

const char *atmosphere_season_label(SeasonId season) {
  switch (season) {
    case SEASON_SPRING_ID: return "Spring rain";
    case SEASON_SUMMER_ID: return "Summer rain";
    case SEASON_AUTUMN_ID: return "Autumn rain";
    case SEASON_WINTER_ID: return "Winter rain";
    default: return "";
  }
}

uint32_t atmosphere_outside_resource(SeasonId season) {
  switch (season) {
    case SEASON_SPRING_ID: return RESOURCE_ID_IMAGE_OUTSIDE_SPRING;
    case SEASON_SUMMER_ID: return RESOURCE_ID_IMAGE_OUTSIDE_SUMMER;
    case SEASON_AUTUMN_ID: return RESOURCE_ID_IMAGE_OUTSIDE_AUTUMN;
    case SEASON_WINTER_ID: return RESOURCE_ID_IMAGE_OUTSIDE_WINTER;
    default: return RESOURCE_ID_IMAGE_OUTSIDE_AUTUMN;
  }
}

GColor atmosphere_sky_tint(TodId tod) {
#if defined(PBL_COLOR)
  switch (tod) {
    case TOD_DAWN_ID:  return GColorMelon;
    case TOD_DAY_ID:   return GColorPictonBlue;
    case TOD_DUSK_ID:  return GColorSunsetOrange;
    case TOD_NIGHT_ID: return GColorOxfordBlue;
    default:           return GColorDarkGray;
  }
#else
  (void)tod;
  return GColorBlack;
#endif
}

GColor atmosphere_night_wash(TodId tod) {
#if defined(PBL_COLOR)
  switch (tod) {
    case TOD_DAWN_ID:  return GColorDarkGray;
    case TOD_DAY_ID:   return GColorWhite;
    case TOD_DUSK_ID:  return GColorDarkCandyAppleRed;
    case TOD_NIGHT_ID: return GColorBlack;
    default:           return GColorDarkGray;
  }
#else
  (void)tod;
  return GColorBlack;
#endif
}

GColor atmosphere_window_bg(TodId tod) {
#if defined(PBL_COLOR)
  switch (tod) {
    case TOD_DAWN_ID:  return GColorDarkGray;
    case TOD_DAY_ID:   return GColorLightGray;
    case TOD_DUSK_ID:  return GColorDarkGray;
    case TOD_NIGHT_ID: return GColorBlack;
    default:           return GColorDarkGray;
  }
#else
  (void)tod;
  return GColorBlack;
#endif
}

GColor atmosphere_condensation_color(int fade_level, TodId tod) {
  if (fade_level < 0) fade_level = 0;
  if (fade_level > 7) fade_level = 7;

#if defined(PBL_COLOR)
  // Fresh wipe is pale / near-white; later levels sink into foggy glass.
  static const GColor DAY_FADE[8] = {
    GColorWhite, GColorWhite, GColorLightGray, GColorLightGray,
    GColorDarkGray, GColorDarkGray, GColorBlack, GColorBlack
  };
  static const GColor NIGHT_FADE[8] = {
    GColorWhite, GColorLightGray, GColorPictonBlue, GColorBlueMoon,
    GColorDukeBlue, GColorOxfordBlue, GColorBlack, GColorBlack
  };
  static const GColor DAWN_FADE[8] = {
    GColorWhite, GColorLightGray, GColorMelon, GColorChromeYellow,
    GColorDarkGray, GColorDarkGray, GColorBlack, GColorBlack
  };
  static const GColor DUSK_FADE[8] = {
    GColorWhite, GColorLightGray, GColorMelon, GColorSunsetOrange,
    GColorDarkCandyAppleRed, GColorDarkGray, GColorBlack, GColorBlack
  };

  switch (tod) {
    case TOD_DAWN_ID:  return DAWN_FADE[fade_level];
    case TOD_DAY_ID:   return DAY_FADE[fade_level];
    case TOD_DUSK_ID:  return DUSK_FADE[fade_level];
    case TOD_NIGHT_ID: return NIGHT_FADE[fade_level];
    default:           return DAY_FADE[fade_level];
  }
#else
  (void)tod;
  // B&W: alternate solid / clear via dither by using white early, black late.
  return (fade_level <= 3) ? GColorWhite : GColorBlack;
#endif
}
