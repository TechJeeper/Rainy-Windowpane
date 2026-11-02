#!/usr/bin/env python3
"""Render Rebble App Store screenshots at exact platform resolutions.

No watch bezel — store requirement. Showcases seasons + times of day
behind rainy condensation glass (max 5 per platform).
"""
from __future__ import annotations

import os
import random
import shutil
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "store" / "screenshots"
OUTSIDE = ROOT / "resources" / "images" / "outside"
FOG = ROOT / "resources" / "images" / "fog_overlay.png"
DROP = ROOT / "resources" / "images" / "raindrop.png"

PLATFORMS = {
    "basalt": (144, 168, True),
    "diorite": (144, 168, False),
    "flint": (144, 168, False),
    "emery": (200, 228, True),
}

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
LIGHT_GRAY = (170, 170, 170)
PICTON = (85, 170, 255)

SCENES = [
    {
        "file": "01-spring-dawn.png",
        "outside": "spring.png",
        "time": "06:42",
        "tod": "dawn",
        "label": None,
        "rain": "medium",
    },
    {
        "file": "02-summer-day.png",
        "outside": "summer.png",
        "time": "14:18",
        "tod": "day",
        "label": None,
        "show_date": True,
        "date": "Sun 2 Nov",
        "rain": "light",
    },
    {
        "file": "03-autumn-dusk.png",
        "outside": "autumn.png",
        "time": "18:05",
        "tod": "dusk",
        "label": "Autumn rain",
        "rain": "heavy",
    },
    {
        "file": "04-winter-night.png",
        "outside": "winter.png",
        "time": "22:47",
        "tod": "night",
        "label": None,
        "rain": "medium",
        "fade": 3,
    },
    {
        "file": "05-spring-night.png",
        "outside": "spring.png",
        "time": "23:11",
        "tod": "night",
        "label": "Spring rain",
        "rain": "heavy",
        "fade": 5,
    },
]


def font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    if bold:
        candidates = [
            "C:/Windows/Fonts/consolab.ttf",
            "C:/Windows/Fonts/lucon.ttf",
            "C:/Windows/Fonts/segoeuib.ttf",
            "C:/Windows/Fonts/arialbd.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        ]
    else:
        candidates = [
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/calibri.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def leco_font(size: int) -> ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/consolab.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "C:/Windows/Fonts/courbd.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return font(size, bold=True)


def draw_text_center(draw: ImageDraw.ImageDraw, text: str, y: int, w: int, fnt, fill) -> None:
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text(((w - tw) // 2, y), text, font=fnt, fill=fill)


def to_bw(img: Image.Image) -> Image.Image:
    gray = img.convert("L")
    bw = gray.point(lambda p: 255 if p > 110 else 0, mode="1").convert("RGB")
    return bw


def apply_tod(scene: Image.Image, tod: str, color: bool) -> Image.Image:
    out = scene.convert("RGBA")
    w, h = out.size
    wash = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(wash)
    band = h // 3
    if tod == "dawn":
        draw.rectangle((0, 0, w, band), fill=(255, 170, 136, 70))
        draw.rectangle((0, h - band, w, h), fill=(255, 136, 85, 50))
    elif tod == "day":
        draw.rectangle((0, 0, w, band), fill=(85, 170, 255, 35))
    elif tod == "dusk":
        draw.rectangle((0, 0, w, band), fill=(255, 85, 0, 70))
        draw.rectangle((0, 0, w, h), fill=(85, 0, 0, 40))
    elif tod == "night":
        draw.rectangle((0, 0, w, h), fill=(0, 0, 34, 110))
        draw.rectangle((0, 0, w, band), fill=(0, 0, 51, 80))
    out = Image.alpha_composite(out, wash)
    if not color:
        return to_bw(out)
    return out.convert("RGB")


def condensation_color(fade: int, tod: str, color: bool) -> tuple[int, int, int]:
    if not color:
        return WHITE if fade <= 3 else BLACK
    if fade <= 1:
        return WHITE
    if fade <= 3:
        return LIGHT_GRAY
    if tod == "night":
        return (85, 85, 170) if fade <= 5 else (0, 0, 51)
    if tod == "dusk":
        return (255, 136, 85) if fade <= 5 else (85, 0, 0)
    return LIGHT_GRAY if fade <= 5 else (51, 51, 51)


def draw_rain(img: Image.Image, intensity: str, color: bool, drop: Image.Image | None) -> None:
    rng = random.Random(42)
    w, h = img.size
    counts = {"off": 0, "light": 8, "medium": 14, "heavy": 22}
    n = counts.get(intensity, 14)
    draw = ImageDraw.Draw(img)
    stroke = PICTON if color else WHITE
    for i in range(n):
        x = rng.randint(2, w - 3)
        y = rng.randint(0, h - 10)
        length = rng.randint(4, 10)
        if drop is not None and (i & 1) and color:
            dw, dh = drop.size
            scale = max(1, w // 72)
            d = drop.resize((dw * scale // 2 or 1, dh * scale // 2 or 1), Image.NEAREST)
            img.paste(d, (x - d.size[0] // 2, y), d)
        else:
            draw.line((x, y, x, y + length), fill=stroke, width=1)


def draw_frame(draw: ImageDraw.ImageDraw, w: int, h: int, color: bool) -> None:
    edge = BLACK if color else WHITE
    draw.rectangle((0, 0, w - 1, h - 1), outline=edge, width=2)
    draw.line((w // 2, 0, w // 2, h), fill=edge, width=1)
    draw.line((0, h // 2, w, h // 2), fill=edge, width=1)


def render(w: int, h: int, color: bool, scene: dict) -> Image.Image:
    outside = Image.open(OUTSIDE / scene["outside"]).convert("RGBA")
    outside = outside.resize((w, h), Image.NEAREST)
    base = apply_tod(outside, scene["tod"], color).convert("RGBA")

    if FOG.exists():
        fog = Image.open(FOG).convert("RGBA").resize((w, h), Image.NEAREST)
        if not color:
            # Keep fog as a light mist on B&W
            fog_l = fog.split()[0]
            fog = Image.merge(
                "RGBA",
                (fog_l, fog_l, fog_l, fog.split()[-1].point(lambda a: a // 2)),
            )
        base = Image.alpha_composite(base, fog)

    img = base.convert("RGB")
    drop = None
    if DROP.exists():
        drop = Image.open(DROP).convert("RGBA")
    draw_rain(img, scene.get("rain", "medium"), color, drop)

    draw = ImageDraw.Draw(img)
    draw_frame(draw, w, h, color)

    fade = int(scene.get("fade", 1))
    time_color = condensation_color(fade, scene["tod"], color)
    time_size = 36 if w >= 200 else 28
    time_y = h // 2 - (32 if w >= 200 else 26)
    draw_text_center(draw, scene["time"], time_y, w, leco_font(time_size), time_color)

    if fade <= 2:
        smear = LIGHT_GRAY if color else WHITE
        sy = time_y + (40 if w >= 200 else 34)
        draw.line((w // 2 - 40, sy, w // 2 + 40, sy), fill=smear, width=1)

    if scene.get("show_date"):
        draw_text_center(
            draw,
            scene.get("date", "Sun 2 Nov"),
            time_y + (44 if w >= 200 else 38),
            w,
            font(max(11, w // 13)),
            time_color,
        )

    if scene.get("label"):
        label_fill = LIGHT_GRAY if color else WHITE
        draw_text_center(
            draw,
            scene["label"],
            h - (22 if w >= 200 else 18),
            w,
            font(max(11, w // 13)),
            label_fill,
        )

    if color:
        # Slight cool glass cast
        img = ImageEnhance.Color(img).enhance(0.95)
    return img


def main() -> None:
    if OUT.exists():
        shutil.rmtree(OUT)
    for plat, (w, h, color) in PLATFORMS.items():
        dest = OUT / plat
        dest.mkdir(parents=True, exist_ok=True)
        for scene in SCENES:
            shot = render(w, h, color, scene)
            path = dest / scene["file"]
            shot.save(path, "PNG", optimize=True)
            print(f"wrote {path.relative_to(ROOT)} ({w}x{h})")
    print("Done:", OUT)


if __name__ == "__main__":
    main()
