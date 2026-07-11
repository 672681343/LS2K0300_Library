# import os
# import re
# import shutil
# from pathlib import Path

# # ========== 配置路径 ==========
# src_folder = r"C:\Users\YY\Desktop\tensorflow\car_shoot\classify_v3\traffic\tank_1_2"
# # 新文件夹会自动创建在同级目录下
# dst_folder = os.path.join(os.path.dirname(src_folder), "sorted_by_time")

# # ========== 提取时间戳 ==========
# def extract_timestamp(filename):
#     match = re.search(r'(\d{8}_\d{6}_\d{3})', filename)
#     return match.group(1) if match else ""

# # ========== 主程序 ==========
# os.makedirs(dst_folder, exist_ok=True)

# # 获取所有 bmp 文件
# files = [f for f in os.listdir(src_folder) if f.lower().endswith('.bmp')]

# # 按时间戳排序
# files.sort(key=extract_timestamp)

# print(f"源文件夹: {src_folder}")
# print(f"目标文件夹: {dst_folder}")
# print(f"共找到 {len(files)} 张 BMP 图片，开始按时间戳排序复制...\n")

# for i, filename in enumerate(files, 1):
#     src_path = os.path.join(src_folder, filename)
#     ts = extract_timestamp(filename)
    
#     # 新文件名：001_原文件名（保留原文件名，前面加序号方便查看顺序）
#     new_name = f"{i:03d}_{filename}"
#     dst_path = os.path.join(dst_folder, new_name)
    
#     shutil.copy2(src_path, dst_path)
#     print(f"{i:4d}. [{ts}]  {filename}")

# print(f"\n完成！已按时间戳排序并复制到: {dst_folder}")








import os
import re

# ========== 配置 ==========
folder = r"C:\Users\YY\Desktop\tensorflow\car_shoot\fist_classify\traffic"
threshold = "20210324_105251_744"  # 时间戳阈值（包含此时间）

# ========== 提取时间戳 ==========
def extract_timestamp(filename): 
    match = re.search(r'(\d{8}_\d{6}_\d{3})', filename)
    return match.group(1) if match else ""

# ========== 主程序 ==========
files = [f for f in os.listdir(folder) if f.lower().endswith('.bmp')]

print(f"阈值时间戳: {threshold}")
print(f"处理文件夹: {folder}\n")
print(f"{'序号':<6} {'原文件名':<60} {'时间戳':<20} {'操作'}")
print("-" * 100)

count = 0
for i, filename in enumerate(files, 1):
    ts = extract_timestamp(filename)
    
    # 判断条件：时间戳 >= 阈值 且 文件名以 traffic_p1_2 开头
    if ts and ts >= threshold and filename.startswith("traffic_p1_2"):
        new_name = "traffic_p1_1" + filename[len("traffic_p1_2"):]
        old_path = os.path.join(folder, filename)
        new_path = os.path.join(folder, new_name)
        
        # 防止目标文件已存在
        if os.path.exists(new_path):
            print(f"{i:<6} {filename:<60} {ts:<20} 跳过（目标文件已存在）")
            continue
        
        os.rename(old_path, new_path)
        print(f"{i:<6} {filename:<60} {ts:<20} 已重命名 -> {new_name}")
        count += 1
    else:
        print(f"{i:<6} {filename:<60} {ts:<20} 跳过")

print(f"\n完成！共处理 {count} 个文件。")