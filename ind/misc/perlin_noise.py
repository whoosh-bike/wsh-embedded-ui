import math

import matplotlib.pyplot as plt
import numpy as np


def cos_interpol(a, b, t):
    ft = t * math.pi
    f = (1 - math.cos(ft)) * 0.5
    return a * (1 - f) + b * f


def fixed_random_gradient(x):
    x = (x << 13) ^ x
    return 1.0 - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7FFFFFFF) / 1073741824.0


def perlin1D(x):
    x0 = int(math.floor(x))
    x1 = x0 + 1

    g0 = fixed_random_gradient(x0)
    g1 = fixed_random_gradient(x1)

    t = x - x0

    v0 = g0 * t
    v1 = g1 * (t - 1)

    return cos_interpol(v0, v1, t)


def perlin_normalize(val):
    return val * 0.5 + 0.5


def perlin_stretch(val, scale):
    return val * scale


def perlin_normalize_and_stretch(val, min, max):
    return min + (val * 0.5 + 0.5) * (max - min)


def wrap_range(x: float, min_val: float, max_val: float) -> float:
    range_size = max_val - min_val
    if range_size == 0:
        return min_val  # избегаем деления на 0
    return (x - min_val) % range_size + min_val


#
points_max = 1000
points_num = points_max * 10

#
perlin_x = np.linspace(0, points_max, points_num)
perlin_orig_y = [perlin1D(x) for x in perlin_x]

#
speed_scale = 0.2
freq = 30
speed = [idx / (freq * speed_scale) for idx in perlin_x]
perlin_scaled_y = [perlin1D(idx) for idx in speed]

#
stretch_factor = 3
perlin_stretched_y = [perlin_stretch(idx, stretch_factor) for idx in perlin_scaled_y]

#
perlin_norm_y = [perlin_normalize(x) for x in perlin_stretched_y]

#
perlin_custom_y = [wrap_range(idx, 0, 1) for idx in perlin_norm_y]


plt.figure(figsize=(10, 10))

plt.subplot(5, 1, 1)
plt.ylim(-1, 1)
plt.title("1D Perlin Noise - original")
plt.plot(perlin_x, perlin_orig_y, label="orig", color="green")
perlin_orig_y_min = min(perlin_orig_y)
perlin_orig_y_max = max(perlin_orig_y)
plt.axhline(perlin_orig_y_max, color="red", linestyle="--", label=f"max: {perlin_orig_y_max:.2f}")
plt.axhline(perlin_orig_y_min, color="blue", linestyle="--", label=f"min: {perlin_orig_y_min:.2f}")
plt.legend()
plt.grid()

plt.subplot(5, 1, 2)
plt.ylim(-1, 1)
plt.title("1D Perlin Noise - scale")
plt.plot(perlin_x, perlin_scaled_y, label="scale", color="green")
perlin_scale_y_min = min(perlin_scaled_y)
perlin_scale_y_max = max(perlin_scaled_y)
plt.axhline(perlin_scale_y_max, color="red", linestyle="--", label=f"max: {perlin_scale_y_max:.2f}")
plt.axhline(
    perlin_scale_y_min, color="blue", linestyle="--", label=f"min: {perlin_scale_y_min:.2f}"
)
plt.legend()
plt.grid()

plt.subplot(5, 1, 3)
plt.ylim(-1, 1)
plt.title("1D Perlin Noise - stretch")
plt.plot(perlin_x, perlin_stretched_y, label="scale", color="green")
perlin_stretch_y_min = min(perlin_stretched_y)
perlin_stretch_y_max = max(perlin_stretched_y)
plt.axhline(
    perlin_stretch_y_max, color="red", linestyle="--", label=f"max: {perlin_stretch_y_max:.2f}"
)
plt.axhline(
    perlin_stretch_y_min, color="blue", linestyle="--", label=f"min: {perlin_stretch_y_min:.2f}"
)
plt.legend()
plt.grid()

plt.subplot(5, 1, 4)
plt.ylim(0, 1)
plt.title("1D Perlin Noise - norm")
plt.plot(perlin_x, perlin_norm_y, label="norm", color="green")
perlin_norm_y_min = min(perlin_norm_y)
perlin_norm_y_max = max(perlin_norm_y)
plt.axhline(perlin_norm_y_max, color="red", linestyle="--", label=f"max: {perlin_norm_y_max:.2f}")
plt.axhline(perlin_norm_y_min, color="blue", linestyle="--", label=f"min: {perlin_norm_y_min:.2f}")
plt.legend()
plt.grid()

plt.subplot(5, 1, 5)
plt.ylim(0, 1)
plt.title("1D Perlin Noise - wrap value")
plt.plot(perlin_x, perlin_custom_y, label="norm", color="green")
perlin_custom_y_min = min(perlin_custom_y)
perlin_custom_y_max = max(perlin_custom_y)
plt.axhline(
    perlin_custom_y_max,
    color="red",
    linestyle="--",
    label=f"max: {perlin_custom_y_max:.2f}",
)
plt.axhline(
    perlin_custom_y_min,
    color="blue",
    linestyle="--",
    label=f"min: {perlin_custom_y_min:.2f}",
)
plt.legend()
plt.grid()


plt.tight_layout()
plt.show()
