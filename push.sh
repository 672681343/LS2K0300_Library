#!/bin/bash

# 获取脚本所在目录并进入
cd "$(dirname "$0")"

VERSION_FILE="version.txt"

# 如果版本文件不存在，初始化为 1.0.2
if [ ! -f "$VERSION_FILE" ]; then
    echo "1.0.2" > "$VERSION_FILE"
fi

# 读取版本并去除空格
CURRENT_VER=$(cat "$VERSION_FILE" | tr -d ' ')

# 解析版本号 (X.Y.Z)
IFS='.' read -r V1 V2 V3 <<< "$CURRENT_VER"

# 版本逻辑递增
V3=$((V3 + 1))
if [ "$V3" -gt 9 ]; then
    V3=0
    V2=$((V2 + 1))
fi

if [ "$V2" -gt 9 ]; then
    V2=0
    V1=$((V1 + 1))
fi

NEXT_VER="V$V1.$V2.$V3"

echo "==========================================="
echo "  当前建议版本: $NEXT_VER"
echo "==========================================="

# 读取用户输入
read -p "请输入修改描述 (直接回车则仅使用版本号): " USER_DESC

# 构建最终提交信息
if [ -z "$USER_DESC" ]; then
    FINAL_MSG="$NEXT_VER"
    # 只有在直接回车时才更新版本文件
    echo "$V1.$V2.$V3" > "$VERSION_FILE"
else
    # 正则判断：如果描述本身就是以 V+数字 开头，则不重复加版本号前缀
    if [[ "$USER_DESC" =~ ^V[0-9] ]]; then
        FINAL_MSG="$USER_DESC"
    else
        FINAL_MSG="$NEXT_VER $USER_DESC"
    fi
fi

echo ""
echo "[1/2] 正在提交: \"$FINAL_MSG\""
git add .
git commit -m "$FINAL_MSG"

echo "[2/2] 正在推送至 GitHub..."
git push origin 北方匈奴

if [ $? -eq 0 ]; then
    echo "==========================================="
    echo "  [SUCCESS] 上传成功！"
    echo "==========================================="
else
    echo ""
    echo "  [ERROR] 上传失败，请检查网络或 Git 冲突。"
fi

echo ""
read -n 1 -s -r -p "请按任意键退出..."
echo ""
