from __future__ import annotations

import json
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import scipy.ndimage
from noise import pnoise2
from PIL import Image

SIZE = 512
WORLD_METERS = 800.0
METERS_PER_PIXEL = WORLD_METERS / SIZE
CENTER = np.array([256.0, 256.0], dtype=np.float32)
OUT_DIR = Path(__file__).resolve().parents[1] / "assets" / "textures" / "terrain"


def fbm(x: np.ndarray, z: np.ndarray, octaves: int, persistence: float = 0.5, lacunarity: float = 2.0, scale: float = 1.0) -> np.ndarray:
    acc = np.zeros_like(x, dtype=np.float32)
    freq = 1.0
    amp = 1.0
    amp_sum = 0.0
    for _ in range(octaves):
        n = np.zeros_like(x, dtype=np.float32)
        for r in range(x.shape[0]):
            for c in range(x.shape[1]):
                n[r, c] = pnoise2(x[r, c] * scale * freq, z[r, c] * scale * freq)
        acc += n * amp
        amp_sum += amp
        amp *= persistence
        freq *= lacunarity
    if amp_sum > 0.0:
        acc /= amp_sum
    return acc


def smoothstep(edge0: float, edge1: float, x: np.ndarray) -> np.ndarray:
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def draw_creek_path(height: np.ndarray, path_points: np.ndarray, depth: float = 0.3, width_px: float = 2.0) -> None:
    h, w = height.shape
    mask = np.zeros_like(height, dtype=np.float32)

    def bezier(t: np.ndarray, p0: np.ndarray, p1: np.ndarray, p2: np.ndarray, p3: np.ndarray) -> np.ndarray:
        return (
            ((1 - t) ** 3)[:, None] * p0
            + (3 * ((1 - t) ** 2) * t)[:, None] * p1
            + (3 * (1 - t) * (t ** 2))[:, None] * p2
            + (t ** 3)[:, None] * p3
        )

    t = np.linspace(0.0, 1.0, 160, dtype=np.float32)
    pts = bezier(t, path_points[0], path_points[1], path_points[2], path_points[3])

    for p in pts:
        cx, cz = int(round(p[0])), int(round(p[1]))
        x0 = max(cx - int(width_px) - 2, 0)
        x1 = min(cx + int(width_px) + 3, w)
        z0 = max(cz - int(width_px) - 2, 0)
        z1 = min(cz + int(width_px) + 3, h)
        zz, xx = np.mgrid[z0:z1, x0:x1]
        d = np.sqrt((xx - p[0]) ** 2 + (zz - p[1]) ** 2)
        local = np.clip(1.0 - d / max(width_px, 1e-3), 0.0, 1.0)
        mask[z0:z1, x0:x1] = np.maximum(mask[z0:z1, x0:x1], local)

    mask = scipy.ndimage.gaussian_filter(mask, sigma=1.0)
    height -= mask * depth


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print("[1/6] Generating base terrain noise")
    z_coords, x_coords = np.mgrid[0:SIZE, 0:SIZE].astype(np.float32)

    base = np.zeros((SIZE, SIZE), dtype=np.float32)
    medium = np.zeros((SIZE, SIZE), dtype=np.float32)
    fine = np.zeros((SIZE, SIZE), dtype=np.float32)

    for z in range(SIZE):
        for x in range(SIZE):
            base[z, x] = pnoise2(x * 0.003, z * 0.003, octaves=1) * 12.0
            medium[z, x] = pnoise2(
                x * 0.008 + 100.0,
                z * 0.008 + 100.0,
                octaves=4,
                persistence=0.5,
                lacunarity=2.0,
            ) * 3.0
            fine[z, x] = pnoise2(
                x * 0.025 + 200.0,
                z * 0.025 + 200.0,
                octaves=6,
                persistence=0.4,
                lacunarity=2.1,
            ) * 0.4

    terrain_noise = base + medium + fine

    print("[2/6] Applying basin and valley shaping")
    dist = np.sqrt((x_coords - CENTER[0]) ** 2 + (z_coords - CENTER[1]) ** 2)

    basin_radius = 96.0
    slope_width = 40.0
    basin_mask = 1.0 - np.clip((dist - basin_radius) / slope_width, 0.0, 1.0)
    basin_mask = basin_mask ** 2.0

    basin_fbm = fbm(x_coords, z_coords, octaves=2, persistence=0.55, lacunarity=2.0, scale=0.01)
    basin_floor = 0.0 + basin_fbm * 0.5

    # Base blend: preserve macro noise outside, flatten center basin.
    height = terrain_noise * (1.0 - basin_mask) + (fine * 0.3 + basin_floor) * basin_mask

    # Valley walls / bowl profile.
    valley_t = smoothstep(96.0, 136.0, dist)
    rise = valley_t * 8.0
    slope_noise = fbm(x_coords + 37.0, z_coords + 91.0, octaves=4, persistence=0.5, lacunarity=2.0, scale=0.012) * 1.5
    height += rise + slope_noise * valley_t

    # Outer forest hills and ridges.
    forest_mask = np.clip((dist - 136.0) / 120.0, 0.0, 1.0)
    forest_noise = fbm(x_coords + 1000.0, z_coords + 1000.0, octaves=6, persistence=0.55, lacunarity=2.0, scale=0.03)
    forest_hills = 8.0 + forest_noise * 6.0
    height = height * (1.0 - forest_mask) + forest_hills * forest_mask

    ridge_zone = np.clip((dist - (SIZE / 2.0 - 60.0)) / 60.0, 0.0, 1.0)
    ridge_src = fbm(x_coords + 1700.0, z_coords + 2100.0, octaves=5, persistence=0.55, lacunarity=2.0, scale=0.05)
    ridged = np.abs(ridge_src) * 2.0 - ridge_src
    height += ridged * 2.0 * ridge_zone

    print("[3/6] Applying special landmarks (pond, quarry, creek)")
    # Pond depression: center (200,300), radius 18px, depth -0.4m smooth bowl.
    pond_center = np.array([200.0, 300.0], dtype=np.float32)
    pond_dist = np.sqrt((x_coords - pond_center[0]) ** 2 + (z_coords - pond_center[1]) ** 2)
    pond_falloff = np.clip(1.0 - pond_dist / 18.0, 0.0, 1.0)
    height -= (0.4 * pond_falloff) ** 2

    # Quarry cut: rectangle around (380,200), size 30x20 px, depth -1.2m.
    qx0, qx1 = 380 - 15, 380 + 15
    qz0, qz1 = 200 - 10, 200 + 10
    height[qz0:qz1, qx0:qx1] -= 1.2
    quarry_mask = np.zeros_like(height)
    quarry_mask[qz0:qz1, qx0:qx1] = 1.0
    quarry_mask = scipy.ndimage.gaussian_filter(quarry_mask, sigma=3.0)
    height -= quarry_mask * 0.15

    # Creek bed via cubic bezier from (120,150) to pond.
    creek_points = np.array([
        [120.0, 150.0],
        [145.0, 165.0],
        [175.0, 240.0],
        [200.0, 300.0],
    ], dtype=np.float32)
    draw_creek_path(height, creek_points, depth=0.3, width_px=2.0)

    print("[4/6] Post-processing")
    height = scipy.ndimage.gaussian_filter(height, sigma=1.2)
    height = np.clip(height, -1.0, 16.0)

    hmin = float(height.min())
    hmax = float(height.max())

    img8 = ((height - hmin) / max(hmax - hmin, 1e-6) * 255.0).astype(np.uint8)
    img16 = ((height - hmin) / max(hmax - hmin, 1e-6) * 65535.0).astype(np.uint16)

    print("[5/6] Saving heightmap and metadata")
    Image.fromarray(img8, mode="L").save(OUT_DIR / "terrain_heights_preview.png")
    Image.fromarray(img16, mode="I;16").save(OUT_DIR / "terrain_heights.png")

    meta = {
        "width_pixels": 512,
        "depth_pixels": 512,
        "world_width_meters": 800.0,
        "world_depth_meters": 800.0,
        "meters_per_pixel": 1.5625,
        "height_min_meters": -1.0,
        "height_max_meters": 16.0,
        "basin_center_pixel": [256, 256],
        "basin_radius_pixels": 96,
        "town_bounds_meters": {"min": [250, 250], "max": [550, 550]},
    }
    (OUT_DIR / "terrain_meta.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")

    print("[6/6] Saving debug 3D visualization")
    fig = plt.figure(figsize=(11, 8))
    ax = fig.add_subplot(111, projection="3d")
    stride = 4
    xs = x_coords[::stride, ::stride]
    zs = z_coords[::stride, ::stride]
    hs = height[::stride, ::stride]
    ax.plot_surface(xs, zs, hs, cmap="terrain", linewidth=0, antialiased=False)
    ax.set_title("Fromville Terrain Heightmap")
    ax.set_xlabel("Pixel X")
    ax.set_ylabel("Pixel Z")
    ax.set_zlabel("Height (m)")
    fig.tight_layout()
    fig.savefig(OUT_DIR / "terrain_debug_3d.png", dpi=160)
    plt.close(fig)

    basin_region = height[160:352, 160:352]
    slope_band = np.logical_and(dist >= 100.0, dist <= 136.0)
    slope_avg = float(np.mean(height[slope_band])) if np.any(slope_band) else 0.0

    print("Generation complete")
    print(f"  min height: {hmin:.3f} m")
    print(f"  max height: {hmax:.3f} m")
    print(f"  basin average: {float(np.mean(basin_region)):.3f} m")
    print(f"  slope average: {slope_avg:.3f} m")


if __name__ == "__main__":
    main()
