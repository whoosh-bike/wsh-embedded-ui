import matplotlib.pyplot as plt
import numpy as np

t = np.linspace(0, 4 * np.pi, 1000)


# 1. Синусоида с задержкой на пиках (используем форму |sin(x)|^power)
def shaped_sine(t, power=3):
    return np.sign(np.sin(t)) * np.abs(np.sin(t)) ** power


# 2. Модифицированная треугольная волна
def triangle_wave_hold(t, power=2):
    saw = (t / (2 * np.pi)) % 1
    tri = 2 * np.abs(saw - 0.5)
    shaped = (1 - tri) ** power
    return 2 * shaped - 1  # масштабируем в диапазон [-1, 1]


# 3. Smootherstep wave
def smootherstep(x):
    return x * x * x * (x * (x * 6 - 15) + 10)


def smootherstep_wave(t):
    phase = (t / (2 * np.pi)) % 1
    # волна вверх-вниз
    value = np.where(phase < 0.5, smootherstep(phase * 2), 1 - smootherstep((phase - 0.5) * 2))
    return 2 * value - 1  # в диапазон [-1, 1]


y1 = shaped_sine(t, power=3)
y2 = triangle_wave_hold(t, power=2)
y3 = smootherstep_wave(t)

plt.figure(figsize=(10, 6))

plt.plot(t, y1, label="Shaped Sine (|sin|^3)")
plt.plot(t, y2, label="Shaped Triangle")
plt.plot(t, y3, label="Smootherstep Wave")

plt.title("Функции с задержкой на пиках и впадинах")
plt.xlabel("Время (t)")
plt.ylabel("Амплитуда")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
