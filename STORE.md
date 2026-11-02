# Rebble Appstore listing — Rainy Windowpane

Copy these fields into [dev-portal.rebble.io](https://dev-portal.rebble.io/submit).

| Field | Value |
| --- | --- |
| App name | Rainy Windowpane |
| Type | Watchface |
| Category | Faces *(automatic)* |
| Website | *(optional)* |
| Source code | https://github.com/TechJeeper/Rainy-Windowpane |
| `.pbw` | `build/RainyWindowpane.pbw` |

## Description

Paste into each platform asset collection (max 1600 characters):

```
Rainy Windowpane looks out on a PixelLab storm through foggy glass.

Pixel raindrops slide down the pane while the time is wiped into the condensation — bright for a moment, then slowly fading until the next minute redraws it.

Outside the window follows your real clock and calendar: dawn, day, dusk, and night light washes over spring, summer, autumn, and winter scenes.

• Sliding pixel rain on foggy glass
• Condensation time that fades each minute
• Season + time-of-day outside the window
• Optional date and season caption
• Clay settings for rain, fade speed, season, and lighting

Color watches show the full rainy palette; B&W watches keep the rain, wipe, and silhouettes.
```

## Release notes (v1.0.0)

```
First release: rainy glass, fading condensation time, seasonal outside scenes with live time-of-day lighting, Clay settings.
```

## Screenshots

Exact platform sizes (no watch bezel). Regenerate with:

```powershell
python tools/generate_store_screenshots.py
```

| Platform | Size | Folder |
| --- | --- | --- |
| basalt (Pebble Time) | 144×168 | `store/screenshots/basalt/` |
| diorite (Pebble 2) | 144×168 | `store/screenshots/diorite/` |
| flint (Pebble 2 Duo) | 144×168 | `store/screenshots/flint/` |
| emery (Pebble Time 2) | 200×228 | `store/screenshots/emery/` |

Five shots per platform:

1. Spring · Dawn  
2. Summer · Day (+date)  
3. Autumn · Dusk  
4. Winter · Night  
5. Spring · Night (faded wipe)

## Listing checklist

- [ ] Screenshots uploaded per platform (up to 5)
- [ ] Description above on each asset collection
- [ ] Source URL set to the GitHub repo
- [ ] Publish `build/RainyWindowpane.pbw` as release 1.0.0
