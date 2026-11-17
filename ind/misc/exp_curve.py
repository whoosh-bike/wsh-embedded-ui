import matplotlib.pyplot as plt
import numpy as np


def exp_curve(x, x_min, x_max, y_min, y_max, k):
    if k == 0:
        return y_min + (y_max - y_min) * (x - x_min) / (x_max - x_min)

    exp_min = np.exp(k * (x_min - x_min))
    exp_max = np.exp(k * (x_max - x_min))
    exp_x = np.exp(k * (x - x_min))

    return y_min + (y_max - y_min) * (exp_x - exp_min) / (exp_max - exp_min)


x_min, x_max = 0, 100
y_min, y_max = 0, 1
x_values = np.linspace(x_min, x_max, 100)

k_values = [-0.3, -0.05, 0, 0.05, 0.3]

plt.figure(figsize=(8, 6))
for k in k_values:
    y_values = exp_curve(x_values, x_min, x_max, y_min, y_max, k)
    plt.plot(x_values, y_values, label=f"k = {k}")

plt.xlabel("x")
plt.ylabel("y")
plt.title("Exponent curves for different k")
plt.legend()
plt.grid()
plt.show()
