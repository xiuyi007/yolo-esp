import cv2
import numpy as np

# 1. 这里填你的图片文件名
IMG_PATH = "pest.jpg" 

# 2. 这里把 ESP32 的日志复制过来
# 格式：(category, score, x1, y1, x2, y2)
detections = [
    (1, 0.731059, 80, 118, 357, 290),
    (1, 0.500000, 136, 0, 416, 112),
    (1, 0.500000, 515, 165, 799, 359),
    (2, 0.500000, 108, 225, 518, 628)
]

# 3. 定义你的类别名字（对应 index 0, 1, 2...）
# 请根据你训练时的 data.yaml 修改这里！
CLASS_NAMES = [
    "Class_0",  # id 0
    "Aphid",    # id 1 (假设是蚜虫)
    "Worm",     # id 2 (假设是肉虫)
    "Mite",     # id 3
    "Thrips"    # id 4
]

# 绘图逻辑
img = cv2.imread(IMG_PATH)
if img is None:
    print(f"Error: 找不到图片 {IMG_PATH}")
    exit()

print(f"图片尺寸: {img.shape[1]}x{img.shape[0]}")

for i, (cat, score, x1, y1, x2, y2) in enumerate(detections):
    # 颜色随机
    color = ((i * 50) % 255, (i * 100) % 255, (i * 200) % 255)
    
    # 画框
    cv2.rectangle(img, (int(x1), int(y1)), (int(x2), int(y2)), color, 2)
    
    # 准备标签文字
    label_name = CLASS_NAMES[cat] if cat < len(CLASS_NAMES) else f"Cat_{cat}"
    label = f"{label_name} {score:.2f}"
    
    # 画文字背景和文字
    t_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 1)[0]
    cv2.rectangle(img, (int(x1), int(y1)), (int(x1) + t_size[0], int(y1) - t_size[1] - 5), color, -1)
    cv2.putText(img, label, (int(x1), int(y1) - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)

# 显示图片
cv2.imshow("Detection Result", img)
cv2.waitKey(0)
cv2.destroyAllWindows()
# 顺便保存一下
cv2.imwrite("result_verified.jpg", img)
print("验证图已保存为 result_verified.jpg")