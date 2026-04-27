from __future__ import annotations

from pathlib import Path

import numpy as np
import scipy.ndimage
from PIL import Image, ImageDraw

SIZE = 512
OUT_DIR = Path(__file__).resolve().parents[1] / "assets" / "textures" / "terrain"
HEIGHTMAP_PATH = OUT_DIR / "terrain_heights.png"
ROAD_MASK_PATH = OUT_DIR / "road_mask.png"

roads = [
    {"type": "tarmac", "width_px": 5, "points": [(256, 50), (256, 462)]},
    {"type": "tarmac", "width_px": 5, "points": [(50, 256), (462, 256)]},
    {"type": "dirt", "width_px": 3, "points": [(256, 256), (180, 190), (140, 150)]},
    {"type": "dirt", "width_px": 3, "points": [(256, 256), (340, 185), (380, 140)]},
    {"type": "dirt", "width_px": 3, "points": [(256, 256), (175, 320), (135, 370)]},
    {"type": "dirt", "width_px": 3, "points": [(256, 256), (340, 330), (385, 385)]},
    {
        "type": "gravel",
        "width_px": 3,
        "points": [(220, 220), (230, 210), (280, 210), (290, 220), (290, 270), (280, 280), (230, 280), (220, 270), (220, 220)],
    },
]


TYPE_VALUE = {
    "tarmac": 255,
    "dirt": 160,
    "gravel": 200,
}


def draw_roads() -> np.ndarray:
    canvas = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(canvas)

    for road in roads:
        pts = road["points"]
        width = int(road["width_px"])
        value = int(TYPE_VALUE[road["type"]])

        for i in range(len(pts) - 1):
            draw.line([pts[i], pts[i + 1]], fill=value, width=width)

        for p in pts:
            r = max(1, width // 2)
            draw.ellipse((p[0] - r, p[1] - r, p[0] + r, p[1] + r), fill=value)

    mask = np.array(canvas, dtype=np.float32)
    mask = scipy.ndimage.gaussian_filter(mask, sigma=0.6)
    return np.clip(mask, 0.0, 255.0)


def carve_heightmap(mask: np.ndarray) -> None:
    if not HEIGHTMAP_PATH.exists():
        raise FileNotFoundError(f"Missing heightmap at {HEIGHTMAP_PATH}")

    img = Image.open(HEIGHTMAP_PATH)
    h16 = np.array(img, dtype=np.uint16)
    if h16.shape != (SIZE, SIZE):
        raise RuntimeError(f"Unexpected heightmap shape: {h16.shape}")

    # Approximate meter conversion using metadata bounds [-1, 16].
    h_norm = h16.astype(np.float32) / 65535.0
    h_m = h_norm * (16.0 - (-1.0)) + (-1.0)

    carve = (mask / 255.0) * 0.08
    h_m -= carve

    h_m = np.clip(h_m, -1.0, 16.0)
    h_norm_new = (h_m - (-1.0)) / 17.0
    h16_new = np.clip(h_norm_new * 65535.0, 0.0, 65535.0).astype(np.uint16)
    Image.fromarray(h16_new, mode="I;16").save(HEIGHTMAP_PATH)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print("[1/3] Drawing road mask")
    mask = draw_roads()

    print("[2/3] Saving road mask")
    Image.fromarray(mask.astype(np.uint8), mode="L").save(ROAD_MASK_PATH)

    print("[3/3] Carving roads into terrain heightmap")
    carve_heightmap(mask)

    print("Road mask generation complete")
    print(f"  saved: {ROAD_MASK_PATH}")
    print(f"  modified: {HEIGHTMAP_PATH}")


if __name__ == "__main__":
    main()
