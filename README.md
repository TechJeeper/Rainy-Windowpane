# Rainy Windowpane (Pebble)

Pixel rain slides down a foggy window. The time is wiped into the condensation and slowly fades until the next minute redraws it.

Outside the glass tracks **real time of day** (dawn / day / dusk / night) and **season** (spring / summer / autumn / winter), using PixelLab landscapes plus a live lighting wash.

## Settings (phone)

Config uses **Clay** (`@rebble/clay`). Open **Rainy Windowpane → Settings** in the Pebble/Rebble app:

- Hour format / leading zero / date
- Rain intensity
- Condensation fade speed
- Season (auto from date, or lock one)
- Time of day (auto from clock, or lock one)
- Optional season caption

## Build

On Windows (Podman or Docker):

```powershell
./build.ps1
```

Output: `build/RainyWindowpane.pbw`

```powershell
./build.ps1 -Phone 192.168.1.23
./build.ps1 -Zip
```

## Layout

```
src/c/              watchface, rain, atmosphere (season + TOD)
src/pkjs/           Clay settings
resources/images/   PixelLab outside scenes, fog overlay, raindrop, menu icon
```
