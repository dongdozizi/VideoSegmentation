import matplotlib.pyplot as plt
from collections import Counter
import time

# 模拟从文件逐行读取
input_file = "motion_vectors.txt"  # 替换为你的实际文件名

def process_line(line):
    """处理一行数据，将其转换为点的列表"""
    data = list(map(int, line.strip().split()))
    points = [(data[i], data[i + 1]) for i in range(0, len(data), 2)]
    return points

def update_plot(points):
    """绘制当前行的散点图，并在点上标注频率"""
    counter = Counter(points)
    x, y = [], []
    plt.clf()  # 清空当前绘图

    # 绘制点和标注频率
    for point, count in counter.items():
        x.append(point[0])
        y.append(point[1])
        plt.scatter(point[0], point[1], color='blue', alpha=0.6, edgecolor='black')
        plt.text(point[0], point[1] + 0.2, str(count), fontsize=10, ha='center', color='red')

    # 设置网格、坐标范围和整数刻度
    plt.xlim(-12, 12)
    plt.ylim(-12, 12)
    plt.xticks(range(-12, 13))
    plt.yticks(range(-12, 13))
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)

    # 图标题和标签
    plt.title('Scatter Plot with Frequency Annotated', fontsize=16)
    plt.xlabel('X', fontsize=14)
    plt.ylabel('Y', fontsize=14)

    plt.pause(0.3)  # 暂停以刷新图像

# 实时读取文件并刷新图像
plt.figure(figsize=(10, 10))

try:
    with open(input_file, "r") as file:
        for line in file:
            points = process_line(line)  # 仅使用当前行的数据
            update_plot(points)  # 每次刷新图像，只显示当前行
            time.sleep(1)  # 模拟每秒读取一行数据
except FileNotFoundError:
    print(f"文件 {input_file} 不存在！")

plt.show()
