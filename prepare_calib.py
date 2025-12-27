import os
import random
import shutil

# --- 请修改以下路径 ---
source_dir = '/Users/togethf/Documents/硕士/RicePestsV3/VOCdevkit/images/val'  # 你的原始图片存放路径
calib_dir = './calibration_data'  # 你想存放校准集的路径
num_samples = 128  # 官方推荐数量

def prepare_calibration_set():
    if not os.path.exists(calib_dir):
        os.makedirs(calib_dir)
        print(f"已创建文件夹: {calib_dir}")

    # 获取所有图片文件名
    all_images = [f for f in os.listdir(source_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
    
    if len(all_images) < num_samples:
        print(f"警告：源目录图片不足 {num_samples} 张，将全部选取。")
        selected_images = all_images
    else:
        # 随机抽取
        selected_images = random.sample(all_images, num_samples)

    print(f"正在复制 {len(selected_images)} 张图像到校准文件夹...")
    
    for img_name in selected_images:
        src_path = os.path.join(source_dir, img_name)
        dst_path = os.path.join(calib_dir, img_name)
        shutil.copy(src_path, dst_path)

    print(f"✅ 完成！校准集已就绪：{os.path.abspath(calib_dir)}")

if __name__ == "__main__":
    prepare_calibration_set()