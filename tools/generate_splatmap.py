from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import scipy.ndimage
from PIL import Image

SIZE = 512
CENTER = np.array([256.0, 256.0], dtype=np.float32)
OUT_DIR = Path(__file__).resolve().parents[1] / "assets" / "textures" / "terrain"
ROAD_MASK_PATH = OUT_DIR / "road_mask.png"
SPLAT_PATH = OUT_DIR / "terrain_splat.png"


def load_road_mask() -> np.ndarray:
    if not ROAD_MASK_PATH.exists():
        raise FileNotFoundError(f"Missing road mask at {ROAD_MASK_PATH}")
    return np.array(Image.open(ROAD_MASK_PATH).convert("L"), dtype=np.float32)


def build_splat(road_mask: np.ndarray) -> np.ndarray:
    z, x = np.mgrid[0:SIZE, 0:SIZE].astype(np.float32)
    dist = np.sqrt((x - CENTER[0]) ** 2 + (z - CENTER[1]) ** 2)

    road_presence = (road_mask > 128.0).astype(np.float32)

    # Grass (R)
    r = np.full((SIZE, SIZE), 200.0, dtype=np.float32)
    r[dist > 110.0] = 60.0
    r[road_presence > 0.0] = 0.0

    # Approximate building proximity by canonical centers.
    building_pts = np.array(
        [
            [220.0, 220.0],
            [256.0, 256.0],
            [180.0, 190.0],
            [340.0, 185.0],
            [175.0, 320.0],
            [340.0, 330.0],
        ],
        dtype=np.float32,
    )
    for bp in building_pts:
        d = np.sqrt((x - bp[0]) ** 2 + (z - bp[1]) ** 2)
        near = np.clip(1.0 - d / 2.5, 0.0, 1.0)
        r = np.minimum(r, 80.0 + (1.0 - near) * 120.0)

    # Road (G)
    g = np.zeros((SIZE, SIZE), dtype=np.float32)
    g[road_mask > 128.0] = 240.0
    g[np.logical_and(road_mask > 64.0, road_mask <= 128.0)] = 160.0
    g = scipy.ndimage.gaussian_filter(g, sigma=1.5)

    # Forest floor / leaf litter (B)
    b = np.zeros((SIZE, SIZE), dtype=np.float32)
    slope_zone = np.clip((dist - 96.0) / 40.0, 0.0, 1.0)
    b = slope_zone * 200.0
    b[dist > 136.0] = 220.0
    b[dist < 96.0] = 0.0
    pond_dist = np.sqrt((x - 200.0) ** 2 + (z - 300.0) ** 2)
    b[pond_dist < 25.0] = 30.0

    # Mud / dirt (A)
    a = np.full((SIZE, SIZE), 10.0, dtype=np.float32)
    a[pond_dist < 22.0] = 180.0

    # Shoulders around roads.
    road_soft = scipy.ndimage.gaussian_filter((road_mask > 128.0).astype(np.float32), sigma=1.0)
    shoulder = np.logical_and(road_soft > 0.08, road_soft <= 0.45)
    a[shoulder] = np.maximum(a[shoulder], 120.0)

    # Creek mask from same control path region.
    creek_seed = np.zeros((SIZE, SIZE), dtype=np.float32)
    for t in np.linspace(0.0, 1.0, 140):
        p0 = np.array([120.0, 150.0])
        p1 = np.array([145.0, 165.0])
        p2 = np.array([175.0, 240.0])
        p3 = np.array([200.0, 300.0])
        p = ((1 - t) ** 3) * p0 + 3 * ((1 - t) ** 2) * t * p1 + 3 * (1 - t) * (t ** 2) * p2 + (t ** 3) * p3
        px, pz = int(round(p[0])), int(round(p[1]))
        if 0 <= px < SIZE and 0 <= pz < SIZE:
            creek_seed[pz, px] = 1.0
    creek_mask = scipy.ndimage.gaussian_filter(creek_seed, sigma=1.2)
    a[creek_mask > 0.08] = np.maximum(a[creek_mask > 0.08], 160.0)

    # Post soft blend.
    r = scipy.ndimage.gaussian_filter(r, sigma=0.8)
    g = scipy.ndimage.gaussian_filter(g, sigma=0.8)
    b = scipy.ndimage.gaussian_filter(b, sigma=0.8)
    a = scipy.ndimage.gaussian_filter(a, sigma=0.8)

    rgba = np.stack(
        [
            np.clip(r, 0.0, 255.0),
            np.clip(g, 0.0, 255.0),
            np.clip(b, 0.0, 255.0),
            np.clip(a, 0.0, 255.0),
        ],
        axis=-1,
    ).astype(np.uint8)

    return rgba


def save_debug_preview(rgba: np.ndarray) -> None:
    fig, axs = plt.subplots(2, 3, figsize=(14, 8))
    axs[0, 0].imshow(rgba[:, :, 0], cmap="Greens")
    axs[0, 0].set_title("R - Grass")
    axs[0, 1].imshow(rgba[:, :, 1], cmap="gray")
    axs[0, 1].set_title("G - Road")
    axs[0, 2].imshow(rgba[:, :, 2], cmap="Oranges")
    axs[0, 2].set_title("B - Forest Floor")
    axs[1, 0].imshow(rgba[:, :, 3], cmap="terrain")
    axs[1, 0].set_title("A - Mud")
    axs[1, 1].imshow(rgba)
    axs[1, 1].set_title("Combined RGBA")
    axs[1, 2].axis("off")
    for ax in axs.ravel():
        ax.axis("off")
    fig.tight_layout()
    fig.savefig(OUT_DIR / "terrain_splat_debug.png", dpi=140)
    plt.close(fig)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print("[1/4] Loading road mask")
    road_mask = load_road_mask()

    print("[2/4] Building splat channels")
    rgba = build_splat(road_mask)

    print("[3/4] Saving terrain_splat.png")
    Image.fromarray(rgba, mode="RGBA").save(SPLAT_PATH)

    print("[4/4] Saving debug preview")
    save_debug_preview(rgba)

    print("Splat map generation complete")
    print(f"  saved: {SPLAT_PATH}")


if __name__ == "__main__":
    main()
