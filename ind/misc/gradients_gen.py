import os

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import colors

# https://matplotlib.org/stable/users/explain/colors/colormaps.html

num_steps = 100
cmaps = [
    "viridis",
    "plasma",
    "cool",
    "hsv",
    "brg",
    "turbo",
]

# Список градиентов, заданных вручную (HEX-цвета)
manual_gradients = {
    "SBER": ["#d0f011", "#22FF00", "#00FF00", "#13b436", "#08b02d", "#16ab7e", "#00AFFF"],
    "PINKY": ["#FF00FF", "#FF69B4", "#DC143C", "#B20071", "#F8E2E9"],
    "BATT": ["#00FF00", "#228B22", "#FFD700", "#FF0000"],
    "BOOK": ["#00FFFF", "#00FFFF", "#48D1CC", "#1E90FF", "#BA55D3", "#E30A42", "#FF0000"],
    "IN_USE": ["#4169E1", "#0000FF", "#0000CD", "#0000FF", "#E0FFFF", "#0000FF"],
    "IN_USE_THROTT_OFF": ["#FFFFFF", "#F4FD42", "#FFFFFF", "#0000FF", "#0000FF", "#0000FF"],
    "IN_USE_BRAKE_ON": ["#FFFFFF", "#FF5C5C", "#FFFFFF", "#0000FF", "#0000FF", "#0000FF"],
}


def rgb_to_hsv(rgb):
    """Конвертирует RGB в HSV, возвращая значения в удобных единицах."""
    hsv = colors.rgb_to_hsv(rgb[:3])
    return hsv[0] * 360, hsv[1] * 100, hsv[2] * 100


def create_c_arr(points, cmap_name):
    """Создаёт строку массива для C-кода."""
    a = f"const WeUi_ColorSchemeHsb_t GRAD_{cmap_name.upper()}[{num_steps}] = {{\n\t"
    b = ",\n\t".join([f"{{{hue:.2f}, {sat:.2f}, {bri:.2f} }}" for hue, sat, bri in points])
    c = "\n};\n\n"
    return a + b + c


def generate_manual_gradient(hex_colors, num_steps):
    """Создаёт градиент на основе всех заданных HEX-цветов."""
    rgb_colors = [colors.to_rgb(c) for c in hex_colors]

    # Создаём кастомный colormap
    cmap = colors.LinearSegmentedColormap.from_list("custom", rgb_colors, N=num_steps)

    # Генерируем линейный градиент
    gradient = np.array([cmap(i / (num_steps - 1))[:3] for i in range(num_steps)])
    hsb_gradient = [rgb_to_hsv(color) for color in gradient]
    return hsb_gradient, cmap


if __name__ == "__main__":
    all_gradients = cmaps + list(manual_gradients.keys())
    num_plots = len(all_gradients)

    fig, axs = plt.subplots(
        num_plots, 2, figsize=(10, num_plots * 0.6), gridspec_kw={"width_ratios": [1, 2]}
    )

    with open(
        os.path.dirname(os.path.abspath(__file__)) + "/../core/" + "weui_ind_gradient_data.h", "w"
    ) as c_file:
        c_file.write("#ifndef __WEUI_IND_GRADIENT_DATA_H\n")
        c_file.write("#define __WEUI_IND_GRADIENT_DATA_H\n\n")
        c_file.write('#include "weui_ind_types.h""\n')
        c_file.write("// clang-format off\n\n")

        # Градиенты по встроенным colormap'ам
        for row_idx, cmap_name in enumerate(cmaps):
            cmap = plt.colormaps[cmap_name]
            gradient = [cmap(i / num_steps) for i in range(num_steps)]
            hsb_gradient = [rgb_to_hsv(color) for color in gradient]
            c_array = create_c_arr(hsb_gradient, cmap_name)
            c_file.write(c_array)

            gradient_rgb = np.array([cmap(i / num_steps) for i in range(num_steps)])
            axs[row_idx, 0].imshow([gradient_rgb], aspect="auto")
            axs[row_idx, 0].set_title(cmap_name, fontsize=8, loc="left")
            axs[row_idx, 0].axis("off")

            h, s, b = zip(*hsb_gradient)
            axs[row_idx, 1].plot(h, label="H", color="red")
            axs[row_idx, 1].plot(s, label="S", color="green")
            axs[row_idx, 1].plot(b, label="B", color="blue")
            axs[row_idx, 1].set_ylim(0, 100)
            axs[row_idx, 1].tick_params(axis="both", labelsize=6)
            axs[row_idx, 1].set_yticks([0, 50, 100])
            if row_idx == 0:
                axs[row_idx, 1].legend(loc="upper right", fontsize=6)

        # Градиенты, заданные вручную
        for row_idx, (name, colors_list) in enumerate(manual_gradients.items(), start=len(cmaps)):
            hsb_gradient, cmap = generate_manual_gradient(colors_list, num_steps)
            c_array = create_c_arr(hsb_gradient, name)
            c_file.write(c_array)

            gradient_rgb = np.array([cmap(i / num_steps) for i in range(num_steps)])
            axs[row_idx, 0].imshow([gradient_rgb], aspect="auto")
            axs[row_idx, 0].set_title(f"Manual: {name}", fontsize=8, loc="left")
            axs[row_idx, 0].axis("off")

            h, s, b = zip(*hsb_gradient)
            axs[row_idx, 1].plot(h, label="H", color="red")
            axs[row_idx, 1].plot(s, label="S", color="green")
            axs[row_idx, 1].plot(b, label="B", color="blue")
            axs[row_idx, 1].set_ylim(0, 100)
            axs[row_idx, 1].tick_params(axis="both", labelsize=6)
            axs[row_idx, 1].set_yticks([0, 50, 100])
            if row_idx == 0:
                axs[row_idx, 1].legend(loc="upper right", fontsize=6)

        c_file.write("#endif /* __WEUI_IND_GRADIENT_DATA_H */\n")

    plt.tight_layout()
    plt.show()
