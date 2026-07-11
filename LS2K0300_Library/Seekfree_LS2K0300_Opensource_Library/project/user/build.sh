#!/bin/bash
# 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了V功能)
#   ./build.sh           -> 弹出交互菜单
#   ./build.sh all       -> 全量部署（清理→编译→上传）
#   ./build.sh buildup   -> 增量编译+上传（日常最常用）
#   ./build.sh build     -> 仅编译（增量）
#   ./build.sh upload    -> 仅上传
#   ./build.sh sync-bmp  -> 下载并删除远程图片
#   ./build.sh count-bmp -> 统计本地图片数量
#   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# ===================== 核心配置区 =====================
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${WORK_DIR}/../out"
USER_DIR="${WORK_DIR}/../user"
REMOTE_IP="192.168.1.104"
REMOTE_USER="root"
REMOTE_PATH="/home/root/"
MAKE_JOBS=$(nproc)
RESERVE_FILE="本文件夹作用.txt"

# 图片相关配置（共用 REMOTE_IP）
LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
REMOTE_BMP_DIR="/tmp"

# Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# 自动分类目标子目录
CLASSIFY_BASE="fist_classify"

# 命令显示/禁用配置
# DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# 格式: 用逗号分隔，如 "2,6" 或 "r,g"
DISABLED_COMMANDS=g
HIDDEN_COMMANDS=
# =================================================================

# ===================== 颜色定义 =====================
CLR_RED='\033[31m'
CLR_GREEN='\033[32m'
CLR_YELLOW='\033[33m'
CLR_BLUE='\033[34m'
CLR_CYAN='\033[36m'
CLR_BOLD='\033[1m'
CLR_RESET='\033[0m'

# ===================== 通用函数 =====================
error_exit() {
    echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
    exit 1
}

info_echo() {
    echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
}

warn_echo() {
    echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
}

# ===================== 编译函数 =====================
do_cmake_make() {
    if [ ! -d "${OUT_DIR}" ]; then
        info_echo "创建编译目录: ${OUT_DIR}"
        mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
    fi

    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    info_echo "执行 cmake: ${USER_DIR}"
    cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

    info_echo "执行 make -j${MAKE_JOBS} ..."
    make -j${MAKE_JOBS} || error_exit "make 编译失败！"

    info_echo "✅ 编译成功"
}

# ===================== 上传函数 =====================
do_upload() {
    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    parent_dir_name=$(basename "$(dirname "$(pwd)")")
    info_echo "待上传: ${parent_dir_name}"

    if [ ! -e "${parent_dir_name}" ]; then
        error_exit "找不到产物: ${parent_dir_name}，请先编译！"
    fi

    info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
    scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
        || error_exit "上传失败，请检查网络/权限！"

    info_echo "✅ 上传完成"
}

# ===================== 全量部署 =====================
mode_all() {
    info_echo "【全量部署】清理 → 编译 → 上传"
    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    info_echo "清理目录，仅保留 ${RESERVE_FILE}"
    find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
        || error_exit "清理失败！"

    do_cmake_make
    do_upload

    info_echo "✅ 全量部署完成！"
}

# ===================== 增量编译+上传 =====================
mode_buildup() {
    info_echo "【增量编译+上传】不清理，编译后自动上传"
    do_cmake_make
    do_upload
    info_echo "✅ 增量编译并上传完成！"
}

# ===================== 仅编译 =====================
mode_build() {
    info_echo "【仅编译】增量编译，不上传"
    do_cmake_make
    info_echo "✅ 增量编译完成！"
}

# ===================== 仅上传 =====================
mode_upload() {
    info_echo "【仅上传】直接 scp 已有产物"
    do_upload
    info_echo "✅ 上传完成！"
}

# ===================== 图片操作函数 =====================
do_count_bmp() {
    if [ ! -d "${LOCAL_BMP_DIR}" ]; then
        echo "0"
        return 1
    fi
    local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
    echo "${count}"
}

show_count_bmp() {
    local local_count=$(do_count_bmp)
    info_echo "本地图片数量: ${local_count} 张"
    info_echo "目录: ${LOCAL_BMP_DIR}"

    # 统计远程图片数量
    local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
    if [ $? -eq 0 ]; then
        info_echo "远程图片数量: ${remote_count} 张"
        info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
    else
        warn_echo "远程统计失败，请检查连接"
    fi
}

do_download_bmp() {
    info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
    if [ ! -d "${LOCAL_BMP_DIR}" ]; then
        mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
    fi

    # 获取远程文件数量（独立命令，避免复杂转义）
    local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
    if [ -z "$remote_count" ] || [ "$remote_count" -eq 0 ]; then
        warn_echo "远程没有 .bmp 文件"
        return 1
    fi

    # 获取远程文件总大小（字节）
    local remote_size=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "stat -c '%s' ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | awk '{sum+=\$1} END {print sum+0}'")
    [ -z "$remote_size" ] && remote_size=0

    # 大小转人类可读（用单引号避免 % 被 bash 解析）
    local size_human="${remote_size} B"
    if [ "$remote_size" -gt 1073741824 ]; then
        size_human=$(awk 'BEGIN {printf "%.2f GB", '"$remote_size"'/1073741824}')
    elif [ "$remote_size" -gt 1048576 ]; then
        size_human=$(awk 'BEGIN {printf "%.2f MB", '"$remote_size"'/1048576}')
    elif [ "$remote_size" -gt 1024 ]; then
        size_human=$(awk 'BEGIN {printf "%.2f KB", '"$remote_size"'/1024}')
    fi

    info_echo "远程共 ${remote_count} 张，约 ${size_human}"

    # 检查是否安装了 pv（Pipe Viewer）
    if command -v pv >/dev/null 2>&1; then
        info_echo "正在下载（显示进度）..."
        ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && tar czf - *.bmp" | pv -s "$remote_size" | tar xzf - -C "${LOCAL_BMP_DIR}/"
    else
        # 无 pv：显示旋转光标 + 后台下载
        info_echo "正在下载...（安装 pv 可显示百分比进度）"
        local spinner='|/-\'
        local i=0
        (
            ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && tar czf - *.bmp" | tar xzf - -C "${LOCAL_BMP_DIR}/"
        ) &
        local pid=$!
        while kill -0 $pid 2>/dev/null; do
            printf "\r  [下载中 %s] " "${spinner:$i:1}"
            i=$(( (i+1) % 4 ))
            sleep 0.2
        done
        wait $pid
        local ret=$?
        printf "\r  [下载完成]    \n"
        if [ $ret -ne 0 ]; then
            return $ret
        fi
    fi

    if [ $? -eq 0 ]; then
        info_echo "✅ 下载完成"
        show_count_bmp
    else
        warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
        return 1
    fi
}

do_clear_bmp() {
    info_echo "正在删除本地 *.bmp ..."
    find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
    info_echo "✅ 本地图片已清空"
    show_count_bmp
}

do_clear_remote_bmp() {
    info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
    if [ $? -eq 0 ]; then
        info_echo "✅ 远程图片已删除"
    else
        warn_echo "远程删除失败"
        return 1
    fi
}

do_sync_bmp_auto() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
    echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
    echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    do_download_bmp || return 1

    info_echo "正在删除远程图片..."
    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
    if [ $? -eq 0 ]; then
        info_echo "✅ 远程图片已删除"
    else
        warn_echo "远程删除可能失败"
    fi

    local count=$(do_count_bmp)
    echo ""
    info_echo "📊 当前本地图片数量: ${count} 张"
    info_echo "🌐 远程IP: ${REMOTE_IP}"
}

do_ping_bmp() {
    info_echo "测试连接 ${REMOTE_IP} ..."
    ping -c 3 "${REMOTE_IP}"
}

# ===================== 命令状态管理 =====================
check_disabled() {
    local cmd="$1"
    [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
    return 1
}

check_hidden() {
    local cmd="$1"
    [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
    return 1
}

cmd_status_text() {
    local cmd="$1"
    if check_hidden "$cmd"; then
        echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
    elif check_disabled "$cmd"; then
        echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
    else
        echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
    fi
}

toggle_command() {
    local cmd="$1"
    local name="$2"
    if check_hidden "$cmd"; then
        # 隐藏 -> 启用（用 ,cmd, 包裹确保单个值也能匹配）
        HIDDEN_COMMANDS=$(echo ",$HIDDEN_COMMANDS," | sed "s/,${cmd},/,/g" | sed 's/^,//' | sed 's/,$//')
        info_echo "✅ [$cmd] $name 已启用"
    elif check_disabled "$cmd"; then
        # 禁用 -> 隐藏
        DISABLED_COMMANDS=$(echo ",$DISABLED_COMMANDS," | sed "s/,${cmd},/,/g" | sed 's/^,//' | sed 's/,$//')
        if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=""; fi
        info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
    else
        # 启用 -> 禁用
        if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS="g"; fi
        warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
    fi
    sed -i "s/DISABLED_COMMANDS=\".*\"/DISABLED_COMMANDS=\"${DISABLED_COMMANDS}\"/" "${BASH_SOURCE[0]}"
    sed -i "s/HIDDEN_COMMANDS=\".*\"/HIDDEN_COMMANDS=\"${HIDDEN_COMMANDS}\"/" "${BASH_SOURCE[0]}"
}

manage_disabled() {
    while true; do
        echo ""
        echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
        echo ""
        echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
        echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
        echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
        echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
        echo ""
        echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
        echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
        echo ""
        echo -e "  按编号切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
        echo ""
        echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
        echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
        echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
        echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
        echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
        echo -e "  ${CLR_GREEN}[r]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
        echo -e "  ${CLR_GREEN}[g]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
        echo ""
        echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
        echo ""
        echo -n "  请选择要切换的命令 [0,2-4,6,7,r,g]: "
        read -n1 -r dchoice
        echo ""
        case "$dchoice" in
            2) toggle_command "2" "全量部署" ;;
            3) toggle_command "3" "仅编译" ;;
            4) toggle_command "4" "仅上传" ;;
            6) toggle_command "6" "删除本地图片" ;;
            7) toggle_command "7" "删除远程图片" ;;
            r|R) toggle_command "r" "远程运行 ./project" ;;
            g|G) toggle_command "g" "生成 Windows 控制脚本" ;;
            0) return ;;
            *) warn_echo "无效输入" ;;
        esac
        echo ""
        echo -n "  按任意键继续..."
        read -n1 -r
        echo ""
    done
}

# ===================== 复制进度条（纯文本，兼容所有终端） =====================
# 刷新间隔：每 PROGRESS_INTERVAL 个文件刷新一次，避免文件过多时刷爆终端
PROGRESS_INTERVAL=50

show_copy_progress() {
    local current=$1
    local total=$2
    if [ $total -eq 0 ]; then
        printf "\r  [准备中... 0/0]"
        return
    fi
    local percent=$((current * 100 / total))
    printf "\r  [复制中 %3d%% | %d/%d 文件]" "$percent" "$current" "$total"
}

# ===================== 查看 car_shoot 分类统计 =====================
do_view_car_shoot() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    local target_dir="${WINDOWS_SHARED_DIR}"
    if [ ! -d "$target_dir" ]; then
        warn_echo "car_shoot 目录不存在: ${target_dir}"
        info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
        return 1
    fi

    local classify_dir="${target_dir}/${CLASSIFY_BASE}"
    if [ ! -d "$classify_dir" ]; then
        warn_echo "分类目录不存在: ${classify_dir}"
        info_echo "请先执行 [w] 自动分类复制到 Windows"
        return 1
    fi

    local categories=("materials" "traffic" "weapon")
    local total=0

    for category in "${categories[@]}"; do
        local dir="${classify_dir}/${category}"
        local add_dir="${classify_dir}/add_${category}"
        local count=0
        local add_count=0

        if [ -d "$dir" ]; then
            count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
        fi
        if [ -d "$add_dir" ]; then
            add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
        fi

        local subtotal=$((count + add_count))
        total=$((total + subtotal))

        echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
        if [ $count -gt 0 ]; then
            echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
        fi
        if [ $add_count -gt 0 ]; then
            echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
        fi
        if [ $subtotal -gt 0 ]; then
            echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
        else
            echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
        fi
        echo ""
    done

    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
    echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
}

# ===================== 远程运行 ./project =====================
do_run_remote() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
    echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
    echo ""

    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

    local ret=$?
    echo ""
    if [ $ret -eq 0 ]; then
        info_echo "✅ 远程项目执行完成"
    else
        warn_echo "⚠️  项目退出码: ${ret}"
        echo ""
        echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
        echo "  1. 项目未上传（先执行编译上传）"
        echo "  2. 远程目录下没有 ./project 可执行文件"
        echo "  3. 权限不足（可能需要 chmod +x project）"
        echo "  4. 项目运行依赖缺失"
    fi
}

# ===================== 生成 Windows 控制脚本 =====================
do_gen_win_bat() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    # 检测共享文件夹（重启后常丢失，自动修复）
    if [ ! -d "/mnt/hgfs" ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
        warn_echo "❌ VMware 共享文件夹未挂载"
        info_echo "尝试自动挂载..."
        sudo mkdir -p /mnt/hgfs 2>/dev/null
        sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
        if [ $? -ne 0 ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
            warn_echo "自动挂载失败"
            info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
            return 1
        fi
        info_echo "✅ 自动挂载成功"
    fi

    # 获取虚拟机 IP
    local vm_ip=$(hostname -I | awk '{print $1}')
    if [ -z "$vm_ip" ]; then
        vm_ip="192.168.229.131"
    fi

    # 确定 Windows 桌面路径
    local win_desktop="/mnt/hgfs/Desktop"
    if [ ! -d "$win_desktop" ]; then
        win_desktop="/mnt/hgfs/car_shoot"
    fi

    # 生成 Windows 批处理文件
    local bat_file="${win_desktop}/run_remote.bat"

    cat > "$bat_file" << EOF
@echo off
chcp 65001 >nul
title Remote Control Tool

:MENU
cls
echo ========================================
echo    Remote Control Tool
echo ========================================
echo.
echo   [1] Run build.sh menu (Linux VM)
echo   [2] Run ./project (Car 192.168.1.104)
echo   [0] Exit
echo.
echo ========================================
set /p choice="Select [0-2]: "

if "%choice%"=="1" goto RUN_BUILD
if "%choice%"=="2" goto RUN_PROJECT
if "%choice%"=="0" goto EXIT
if "%choice%"=="" goto MENU
goto MENU

:RUN_BUILD
echo.
echo Connecting to VM to run build.sh...
echo.
ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
echo.
echo Press any key to return...
pause > nul
goto MENU

:RUN_PROJECT
echo.
echo Connecting to Car to run ./project...
echo.
ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
echo.
echo Press any key to return...
pause > nul
goto MENU

:EXIT
exit
EOF

    # 转换换行符 LF -> CRLF（Windows批处理必需）
    sed -i 's/$/\r/' "$bat_file"

    if [ $? -eq 0 ]; then
        info_echo "✅ Windows 批处理脚本已生成"
        echo ""
        echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
        echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
        echo ""
        echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
        echo "  1. 切换到 Windows 桌面"
        echo "  2. 双击 run_remote.bat"
        echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
        echo "  4. 按 2 运行 ./project（连接小车）"
        echo ""
        echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
        echo "  右键 run_remote.bat → 固定到任务栏"
        echo "  以后按 Win+数字 一键启动"
        echo ""
        echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
        echo "  Windows 需安装 OpenSSH 客户端"
    else
        warn_echo "❌ 生成失败"
    fi
}

set_remote_ip() {
    echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
    read -r new_ip
    if [ -n "$new_ip" ]; then
        sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
        REMOTE_IP="$new_ip"
        info_echo "✅ 已更新远程IP为: ${new_ip}"
        info_echo "（重新运行脚本生效）"
    else
        warn_echo "IP 未修改"
    fi
}

show_bmp_config() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
    echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
    echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
    echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
    echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
    echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
    echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
    echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
    echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
    echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
}

# ===================== 自动分类复制到 Windows =====================
do_copy_to_windows() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    # 检测共享文件夹挂载点（重启后常丢失，自动修复）
    local hgfs_mounted=false
    if [ -d "/mnt/hgfs" ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
        hgfs_mounted=true
    fi

    if [ "$hgfs_mounted" = false ]; then
        warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
        info_echo "尝试自动挂载..."
        sudo mkdir -p /mnt/hgfs 2>/dev/null
        sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
        if [ $? -eq 0 ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
            info_echo "✅ 自动挂载成功"
            hgfs_mounted=true
        else
            warn_echo "自动挂载失败"
            echo ""
            echo -e "${CLR_YELLOW}手动修复方法：${CLR_RESET}"
            echo "  sudo mkdir -p /mnt/hgfs"
            echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
            echo ""
            echo -e "${CLR_YELLOW}如果仍失败，请检查 VMware 设置：${CLR_RESET}"
            echo "  虚拟机 → 设置 → 选项 → 共享文件夹 → 总是启用"
            return 1
        fi
    fi

    # 检测目标目录（软链接或实际目录）
    local target_dir="${WINDOWS_SHARED_DIR}"
    if [ ! -d "${target_dir}" ]; then
        warn_echo "目标目录不存在: ${target_dir}"
        info_echo "尝试创建..."
        mkdir -p "${target_dir}" 2>/dev/null
        if [ $? -ne 0 ]; then
            warn_echo "无法创建，尝试查找可用的共享文件夹..."
            echo ""
            echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
            ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
            echo ""
            echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
            read -r target_dir
            if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
                warn_echo "路径无效，取消操作"
                return 1
            fi
        fi
    fi

    # 先确保本地有图片（如果本地没有，先从远程下载）
    local local_count=$(do_count_bmp)
    if [ "${local_count}" -eq 0 ]; then
        info_echo "本地没有图片，先从远程下载..."
        do_download_bmp || return 1
    fi

    # 分类目标根目录
    local classify_dir="${target_dir}/${CLASSIFY_BASE}"
    mkdir -p "${classify_dir}" 2>/dev/null

    echo ""
    info_echo "源: ${LOCAL_BMP_DIR}"
    info_echo "目标: ${classify_dir}"
    echo ""

    # 定义分类规则
    local categories=("materials" "traffic" "weapon")
    local total_copied=0
    local total_add_copied=0

    # 用于存储统计信息
    declare -A old_counts
    declare -A new_counts
    declare -A add_old_counts
    declare -A add_new_counts

    # 先统计各类别复制前的数量
    for category in "${categories[@]}"; do
        local dest_dir="${classify_dir}/${category}"
        local add_dest_dir="${classify_dir}/add_${category}"
        mkdir -p "${dest_dir}" "${add_dest_dir}"
        old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
        add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
    done

    # 预扫描：统计本次需要复制的总文件数
    local total_files=0
    for category in "${categories[@]}"; do
        for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
            [ -e "$file" ] || continue
            local basename=$(basename "$file")
            [[ "$basename" == add_* ]] && continue
            ((total_files++))
        done
        for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
            [ -e "$file" ] || continue
            ((total_files++))
        done
    done

    if [ $total_files -eq 0 ]; then
        warn_echo "没有需要复制的文件"
        return 1
    fi

    echo ""
    info_echo "开始自动分类复制... (${total_files} 个文件)"
    echo ""
    local progress_count=0
    show_copy_progress 0 $total_files

    # 执行复制
    for category in "${categories[@]}"; do
        local count=0
        local add_count=0

        # 旧格式: category_*.bmp（排除 add_ 前缀）
        local dest_dir="${classify_dir}/${category}"

        for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
            [ -e "$file" ] || continue
            local basename=$(basename "$file")
            # 跳过 add_ 前缀的文件
            if [[ "$basename" == add_* ]]; then
                continue
            fi
            cp "$file" "${dest_dir}/"
            ((count++))
            ((progress_count++))
            if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
                show_copy_progress $progress_count $total_files
            fi
        done

        # 增补格式: add_category_*.bmp
        local add_dest_dir="${classify_dir}/add_${category}"

        for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
            [ -e "$file" ] || continue
            cp "$file" "${add_dest_dir}/"
            ((add_count++))
            ((progress_count++))
            if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
                show_copy_progress $progress_count $total_files
            fi
        done

        # 统计复制后的数量
        new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
        add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

        total_copied=$((total_copied + count))
        total_add_copied=$((total_add_copied + add_count))
    done
    echo ""  # 进度条换行

    # 统计未分类的文件
    local uncategorized=0
    for file in "${LOCAL_BMP_DIR}"/*.bmp; do
        [ -e "$file" ] || continue
        local basename=$(basename "$file")
        local classified=false
        for category in "${categories[@]}"; do
            if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
                classified=true
                break
            fi
        done
        if [ "$classified" = false ]; then
            ((uncategorized++))
        fi
    done

    # 显示详细统计
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    for category in "${categories[@]}"; do
        local old=${old_counts[$category]}
        local new=${new_counts[$category]}
        local added=$((new - old))
        local add_old=${add_old_counts[$category]}
        local add_new=${add_new_counts[$category]}
        local add_added=$((add_new - add_old))

        echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
        if [ $added -gt 0 ]; then
            echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
        elif [ $new -gt 0 ]; then
            echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
        fi

        if [ $add_added -gt 0 ]; then
            echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
        elif [ $add_new -gt 0 ]; then
            echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
        fi
        echo ""
    done

    if [ $uncategorized -gt 0 ]; then
        echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
        echo ""
    fi

    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
    if [ $total_add_copied -gt 0 ]; then
        echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
    fi
    echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""
    info_echo "✅ 复制完成！Windows 桌面可直接查看"

    # 询问是否删除本地图片
    echo -n "  是否删除本地已复制的图片？[y/N]: "
    read -n1 -r confirm
    echo ""
    if [[ "$confirm" == [yY] ]]; then
        for category in "${categories[@]}"; do
            rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
            rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
        done
        info_echo "✅ 本地图片已清理"
        show_count_bmp
    fi
}

# ===================== 交互主菜单（动态隐藏，无子菜单） =====================
show_menu() {
    while true; do
        echo ""
        echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
        echo ""
        echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
        echo ""
        echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
        echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
        check_hidden "2" || echo -e "  [2] 全量部署（清理→编译→上传）"
        check_hidden "3" || echo -e "  [3] 仅编译（增量，不上传）"
        check_hidden "4" || echo -e "  [4] 仅上传（不编译）"
        echo ""
        echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
        echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
        check_hidden "6" || echo -e "  [6] 删除本地图片（image_shoot）"
        check_hidden "7" || echo -e "  [7] 删除远程图片（/tmp）"
        echo -e "  [8] 查看本地+远程图片数量"
        echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
        echo ""
        echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
        echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
        check_hidden "r" || echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
        check_hidden "g" || echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
        echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
        echo -e "  [c] 查看当前配置"
        echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
        echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
        echo ""
        echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
        echo ""
        echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
        echo ""
        echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

        read -n1 -r choice
        echo ""

        case "$choice" in
            1) mode_buildup ;;
            2) 
                if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
                elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
                else mode_all; fi ;;
            3) 
                if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
                elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
                else mode_build; fi ;;
            4) 
                if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
                elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
                else mode_upload; fi ;;
            5) do_download_bmp ;;
            6) 
                if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
                elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
                else do_clear_bmp; fi ;;
            7) 
                if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
                elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
                else do_clear_remote_bmp; fi ;;
            8) show_count_bmp ;;
            9) do_sync_bmp_auto ;;
            p|P) do_ping_bmp ;;
            r|R) 
                if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
                elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
                else do_run_remote; fi ;;
            g|G) 
                if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
                elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
                else do_gen_win_bat; fi ;;
            v|V) do_view_car_shoot ;;
            d|D) manage_disabled ;;
            i|I) set_remote_ip ;;
            c|C) show_bmp_config ;;
            w|W) do_copy_to_windows ;;
            0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
            *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
        esac

        echo ""
        echo -n "  按任意键继续..."
        read -n1 -r
        echo ""
    done
}

# ===================== 入口：参数解析 =====================
MODE="${1:-menu}"

case "$MODE" in
    all)        mode_all ;;
    buildup|bu) mode_buildup ;;
    build)      mode_build ;;
    upload|up)  mode_upload ;;

    # 图片相关命令
    download-bmp)   do_download_bmp ;;
    clear-bmp)      do_clear_bmp ;;
    clear-remote-bmp) do_clear_remote_bmp ;;
    count-bmp)      show_count_bmp ;;
    sync-bmp-auto)  do_sync_bmp_auto ;;
    sync-bmp)       do_sync_bmp_auto ;;
    ping-bmp)       do_ping_bmp ;;
    run-remote)     do_run_remote ;;
    gen-win-bat)    do_gen_win_bat ;;
    copy-win)       do_copy_to_windows ;;
    view-car-shoot) do_view_car_shoot ;;

    # 其他
    config)         show_bmp_config ;;
    set-ip)         set_remote_ip ;;

    menu|"")    show_menu ;;
    *)
        echo ""
        echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
        echo ""
        echo "编译部署:"
        echo "  ./build.sh [all|buildup|build|upload]"
        echo ""
        echo "图片工具（Linux）:"
        echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
        echo "  ./build.sh clear-bmp         删除本地图片"
        echo "  ./build.sh clear-remote-bmp  删除远程图片"
        echo "  ./build.sh count-bmp         查看本地图片数量"
        echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
        echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
        echo ""
        echo "图片工具（Windows）:"
        echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
        echo ""
        echo "其他:"
        echo "  ./build.sh ping-bmp          测试远程连接"
        echo "  ./build.sh config            查看当前配置"
        echo "  ./build.sh set-ip            修改远程IP"
        echo "  ./build.sh                   启动交互菜单"
        echo ""
        exit 1
        ;;
esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了V功能)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi

#     # 获取远程文件数量（独立命令，避免复杂转义）
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ -z "$remote_count" ] || [ "$remote_count" -eq 0 ]; then
#         warn_echo "远程没有 .bmp 文件"
#         return 1
#     fi

#     # 获取远程文件总大小（字节）
#     local remote_size=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "stat -c '%s' ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | awk '{sum+=\$1} END {print sum+0}'")
#     [ -z "$remote_size" ] && remote_size=0

#     # 大小转人类可读（用单引号避免 % 被 bash 解析）
#     local size_human="${remote_size} B"
#     if [ "$remote_size" -gt 1073741824 ]; then
#         size_human=$(awk 'BEGIN {printf "%.2f GB", '"$remote_size"'/1073741824}')
#     elif [ "$remote_size" -gt 1048576 ]; then
#         size_human=$(awk 'BEGIN {printf "%.2f MB", '"$remote_size"'/1048576}')
#     elif [ "$remote_size" -gt 1024 ]; then
#         size_human=$(awk 'BEGIN {printf "%.2f KB", '"$remote_size"'/1024}')
#     fi

#     info_echo "远程共 ${remote_count} 张，约 ${size_human}"

#     # 检查是否安装了 pv（Pipe Viewer）
#     if command -v pv >/dev/null 2>&1; then
#         info_echo "正在下载（显示进度）..."
#         ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && tar czf - *.bmp" | pv -s "$remote_size" | tar xzf - -C "${LOCAL_BMP_DIR}/"
#     else
#         # 无 pv：显示旋转光标 + 后台下载
#         info_echo "正在下载...（安装 pv 可显示百分比进度）"
#         local spinner='|/-\'
#         local i=0
#         (
#             ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && tar czf - *.bmp" | tar xzf - -C "${LOCAL_BMP_DIR}/"
#         ) &
#         local pid=$!
#         while kill -0 $pid 2>/dev/null; do
#             printf "\r  [下载中 %s] " "${spinner:$i:1}"
#             i=$(( (i+1) % 4 ))
#             sleep 0.2
#         done
#         wait $pid
#         local ret=$?
#         printf "\r  [下载完成]    \n"
#         if [ $ret -ne 0 ]; then
#             return $ret
#         fi
#     fi

#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_hidden "$cmd"; then
#         # 隐藏 -> 启用（用 ,cmd, 包裹确保单个值也能匹配）
#         HIDDEN_COMMANDS=$(echo ",$HIDDEN_COMMANDS," | sed "s/,${cmd},/,/g" | sed 's/^,//' | sed 's/,$//')
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         # 禁用 -> 隐藏
#         DISABLED_COMMANDS=$(echo ",$DISABLED_COMMANDS," | sed "s/,${cmd},/,/g" | sed 's/^,//' | sed 's/,$//')
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=""; fi
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         # 启用 -> 禁用
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS="g"; fi
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi
#     sed -i "s/DISABLED_COMMANDS=\".*\"/DISABLED_COMMANDS=\"${DISABLED_COMMANDS}\"/" "${BASH_SOURCE[0]}"
#     sed -i "s/HIDDEN_COMMANDS=\".*\"/HIDDEN_COMMANDS=\"${HIDDEN_COMMANDS}\"/" "${BASH_SOURCE[0]}"
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按编号切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[r]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[g]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0,2-4,6,7,r,g]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             2) toggle_command "2" "全量部署" ;;
#             3) toggle_command "3" "仅编译" ;;
#             4) toggle_command "4" "仅上传" ;;
#             6) toggle_command "6" "删除本地图片" ;;
#             7) toggle_command "7" "删除远程图片" ;;
#             r|R) toggle_command "r" "远程运行 ./project" ;;
#             g|G) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 复制进度条（纯文本，兼容所有终端） =====================
# # 刷新间隔：每 PROGRESS_INTERVAL 个文件刷新一次，避免文件过多时刷爆终端
# PROGRESS_INTERVAL=50

# show_copy_progress() {
#     local current=$1
#     local total=$2
#     if [ $total -eq 0 ]; then
#         printf "\r  [准备中... 0/0]"
#         return
#     fi
#     local percent=$((current * 100 / total))
#     printf "\r  [复制中 %3d%% | %d/%d 文件]" "$percent" "$current" "$total"
# }

# # ===================== 查看 car_shoot 分类统计 =====================
# do_view_car_shoot() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "$target_dir" ]; then
#         warn_echo "car_shoot 目录不存在: ${target_dir}"
#         info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
#         return 1
#     fi

#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     if [ ! -d "$classify_dir" ]; then
#         warn_echo "分类目录不存在: ${classify_dir}"
#         info_echo "请先执行 [w] 自动分类复制到 Windows"
#         return 1
#     fi

#     local categories=("materials" "traffic" "weapon")
#     local total=0

#     for category in "${categories[@]}"; do
#         local dir="${classify_dir}/${category}"
#         local add_dir="${classify_dir}/add_${category}"
#         local count=0
#         local add_count=0

#         if [ -d "$dir" ]; then
#             count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
#         fi
#         if [ -d "$add_dir" ]; then
#             add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
#         fi

#         local subtotal=$((count + add_count))
#         total=$((total + subtotal))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $count -gt 0 ]; then
#             echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
#         fi
#         if [ $add_count -gt 0 ]; then
#             echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
#         fi
#         if [ $subtotal -gt 0 ]; then
#             echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
#         else
#             echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
#         fi
#         echo ""
#     done

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
#     echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹（重启后常丢失，自动修复）
#     if [ ! -d "/mnt/hgfs" ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -ne 0 ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             warn_echo "自动挂载失败"
#             info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             return 1
#         fi
#         info_echo "✅ 自动挂载成功"
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点（重启后常丢失，自动修复）
#     local hgfs_mounted=false
#     if [ -d "/mnt/hgfs" ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         hgfs_mounted=true
#     fi

#     if [ "$hgfs_mounted" = false ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -eq 0 ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             info_echo "✅ 自动挂载成功"
#             hgfs_mounted=true
#         else
#             warn_echo "自动挂载失败"
#             echo ""
#             echo -e "${CLR_YELLOW}手动修复方法：${CLR_RESET}"
#             echo "  sudo mkdir -p /mnt/hgfs"
#             echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             echo ""
#             echo -e "${CLR_YELLOW}如果仍失败，请检查 VMware 设置：${CLR_RESET}"
#             echo "  虚拟机 → 设置 → 选项 → 共享文件夹 → 总是启用"
#             return 1
#         fi
#     fi

#     # 检测目标目录（软链接或实际目录）
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 预扫描：统计本次需要复制的总文件数
#     local total_files=0
#     for category in "${categories[@]}"; do
#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             [[ "$basename" == add_* ]] && continue
#             ((total_files++))
#         done
#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             ((total_files++))
#         done
#     done

#     if [ $total_files -eq 0 ]; then
#         warn_echo "没有需要复制的文件"
#         return 1
#     fi

#     echo ""
#     info_echo "开始自动分类复制... (${total_files} 个文件)"
#     echo ""
#     local progress_count=0
#     show_copy_progress 0 $total_files

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#             ((progress_count++))
#             if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
#                 show_copy_progress $progress_count $total_files
#             fi
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#             ((progress_count++))
#             if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
#                 show_copy_progress $progress_count $total_files
#             fi
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done
#     echo ""  # 进度条换行

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             v|V) do_view_car_shoot ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;
#     view-car-shoot) do_view_car_shoot ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac








# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(更快下载)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi

#     # 高速方案：远程 tar 打包 → 管道直传 → 本地解压（单连接，比 scp 逐个文件快 5~10 倍）
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ "$remote_count" -eq 0 ]; then
#         warn_echo "远程没有 .bmp 文件"
#         return 1
#     fi

#     info_echo "远程共 ${remote_count} 张，正在打包传输..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && tar czf - *.bmp" | tar xzf - -C "${LOCAL_BMP_DIR}/"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_hidden "$cmd"; then
#         HIDDEN_COMMANDS=$(echo "$HIDDEN_COMMANDS" | sed "s/,${cmd}//g" | sed 's/^,//' | sed 's/,$//')
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         DISABLED_COMMANDS=$(echo "$DISABLED_COMMANDS" | sed "s/,${cmd}//g" | sed 's/^,//' | sed 's/,$//')
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=""; fi
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS="g"; fi
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi
#     sed -i "s/DISABLED_COMMANDS=\".*\"/DISABLED_COMMANDS=\"${DISABLED_COMMANDS}\"/" "${BASH_SOURCE[0]}"
#     sed -i "s/HIDDEN_COMMANDS=\".*\"/HIDDEN_COMMANDS=\"${HIDDEN_COMMANDS}\"/" "${BASH_SOURCE[0]}"
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按数字切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 复制进度条（纯文本，兼容所有终端） =====================
# # 刷新间隔：每 PROGRESS_INTERVAL 个文件刷新一次，避免文件过多时刷爆终端
# PROGRESS_INTERVAL=50

# show_copy_progress() {
#     local current=$1
#     local total=$2
#     if [ $total -eq 0 ]; then
#         printf "\r  [准备中... 0/0]"
#         return
#     fi
#     local percent=$((current * 100 / total))
#     printf "\r  [复制中 %3d%% | %d/%d 文件]" "$percent" "$current" "$total"
# }

# # ===================== 查看 car_shoot 分类统计 =====================
# do_view_car_shoot() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "$target_dir" ]; then
#         warn_echo "car_shoot 目录不存在: ${target_dir}"
#         info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
#         return 1
#     fi

#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     if [ ! -d "$classify_dir" ]; then
#         warn_echo "分类目录不存在: ${classify_dir}"
#         info_echo "请先执行 [w] 自动分类复制到 Windows"
#         return 1
#     fi

#     local categories=("materials" "traffic" "weapon")
#     local total=0

#     for category in "${categories[@]}"; do
#         local dir="${classify_dir}/${category}"
#         local add_dir="${classify_dir}/add_${category}"
#         local count=0
#         local add_count=0

#         if [ -d "$dir" ]; then
#             count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
#         fi
#         if [ -d "$add_dir" ]; then
#             add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
#         fi

#         local subtotal=$((count + add_count))
#         total=$((total + subtotal))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $count -gt 0 ]; then
#             echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
#         fi
#         if [ $add_count -gt 0 ]; then
#             echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
#         fi
#         if [ $subtotal -gt 0 ]; then
#             echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
#         else
#             echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
#         fi
#         echo ""
#     done

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
#     echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹（重启后常丢失，自动修复）
#     if [ ! -d "/mnt/hgfs" ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -ne 0 ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             warn_echo "自动挂载失败"
#             info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             return 1
#         fi
#         info_echo "✅ 自动挂载成功"
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点（重启后常丢失，自动修复）
#     local hgfs_mounted=false
#     if [ -d "/mnt/hgfs" ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         hgfs_mounted=true
#     fi

#     if [ "$hgfs_mounted" = false ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -eq 0 ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             info_echo "✅ 自动挂载成功"
#             hgfs_mounted=true
#         else
#             warn_echo "自动挂载失败"
#             echo ""
#             echo -e "${CLR_YELLOW}手动修复方法：${CLR_RESET}"
#             echo "  sudo mkdir -p /mnt/hgfs"
#             echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             echo ""
#             echo -e "${CLR_YELLOW}如果仍失败，请检查 VMware 设置：${CLR_RESET}"
#             echo "  虚拟机 → 设置 → 选项 → 共享文件夹 → 总是启用"
#             return 1
#         fi
#     fi

#     # 检测目标目录（软链接或实际目录）
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 预扫描：统计本次需要复制的总文件数
#     local total_files=0
#     for category in "${categories[@]}"; do
#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             [[ "$basename" == add_* ]] && continue
#             ((total_files++))
#         done
#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             ((total_files++))
#         done
#     done

#     if [ $total_files -eq 0 ]; then
#         warn_echo "没有需要复制的文件"
#         return 1
#     fi

#     echo ""
#     info_echo "开始自动分类复制... (${total_files} 个文件)"
#     echo ""
#     local progress_count=0
#     show_copy_progress 0 $total_files

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#             ((progress_count++))
#             if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
#                 show_copy_progress $progress_count $total_files
#             fi
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#             ((progress_count++))
#             if [ $((progress_count % PROGRESS_INTERVAL)) -eq 0 ] || [ $progress_count -eq $total_files ]; then
#                 show_copy_progress $progress_count $total_files
#             fi
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done
#     echo ""  # 进度条换行

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             v|V) do_view_car_shoot ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;
#     view-car-shoot) do_view_car_shoot ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了V功能)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_hidden "$cmd"; then
#         HIDDEN_COMMANDS=$(echo "$HIDDEN_COMMANDS" | sed "s/,${cmd}//g" | sed 's/^,//' | sed 's/,$//')
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         DISABLED_COMMANDS=$(echo "$DISABLED_COMMANDS" | sed "s/,${cmd}//g" | sed 's/^,//' | sed 's/,$//')
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=""; fi
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS="g"; fi
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi
#     sed -i "s/DISABLED_COMMANDS="g"
#     sed -i "s/HIDDEN_COMMANDS=""
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按数字切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 复制进度条 =====================
# show_copy_progress() {
#     local current=$1
#     local total=$2
#     local width=30
#     if [ $total -eq 0 ]; then
#         printf "\r  [░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]   0%% (0/0)"
#         return
#     fi
#     local percent=$((current * 100 / total))
#     local filled=$((current * width / total))
#     local empty=$((width - filled))

#     printf "\r  ["
#     printf "%${filled}s" | tr ' ' '█'
#     printf "%${empty}s" | tr ' ' '░'
#     printf "] %3d%% (%d/%d)" "$percent" "$current" "$total"
# }

# # ===================== 查看 car_shoot 分类统计 =====================
# do_view_car_shoot() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "$target_dir" ]; then
#         warn_echo "car_shoot 目录不存在: ${target_dir}"
#         info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
#         return 1
#     fi

#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     if [ ! -d "$classify_dir" ]; then
#         warn_echo "分类目录不存在: ${classify_dir}"
#         info_echo "请先执行 [w] 自动分类复制到 Windows"
#         return 1
#     fi

#     local categories=("materials" "traffic" "weapon")
#     local total=0

#     for category in "${categories[@]}"; do
#         local dir="${classify_dir}/${category}"
#         local add_dir="${classify_dir}/add_${category}"
#         local count=0
#         local add_count=0

#         if [ -d "$dir" ]; then
#             count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
#         fi
#         if [ -d "$add_dir" ]; then
#             add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
#         fi

#         local subtotal=$((count + add_count))
#         total=$((total + subtotal))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $count -gt 0 ]; then
#             echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
#         fi
#         if [ $add_count -gt 0 ]; then
#             echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
#         fi
#         if [ $subtotal -gt 0 ]; then
#             echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
#         else
#             echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
#         fi
#         echo ""
#     done

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
#     echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹（重启后常丢失，自动修复）
#     if [ ! -d "/mnt/hgfs" ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -ne 0 ] || [ -z "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             warn_echo "自动挂载失败"
#             info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             return 1
#         fi
#         info_echo "✅ 自动挂载成功"
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点（重启后常丢失，自动修复）
#     local hgfs_mounted=false
#     if [ -d "/mnt/hgfs" ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#         hgfs_mounted=true
#     fi

#     if [ "$hgfs_mounted" = false ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         info_echo "尝试自动挂载..."
#         sudo mkdir -p /mnt/hgfs 2>/dev/null
#         sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000 2>/dev/null
#         if [ $? -eq 0 ] && [ -n "$(ls -A /mnt/hgfs 2>/dev/null)" ]; then
#             info_echo "✅ 自动挂载成功"
#             hgfs_mounted=true
#         else
#             warn_echo "自动挂载失败"
#             echo ""
#             echo -e "${CLR_YELLOW}手动修复方法：${CLR_RESET}"
#             echo "  sudo mkdir -p /mnt/hgfs"
#             echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#             echo ""
#             echo -e "${CLR_YELLOW}如果仍失败，请检查 VMware 设置：${CLR_RESET}"
#             echo "  虚拟机 → 设置 → 选项 → 共享文件夹 → 总是启用"
#             return 1
#         fi
#     fi

#     # 检测目标目录（软链接或实际目录）
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 预扫描：统计本次需要复制的总文件数
#     local total_files=0
#     for category in "${categories[@]}"; do
#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             [[ "$basename" == add_* ]] && continue
#             ((total_files++))
#         done
#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             ((total_files++))
#         done
#     done

#     if [ $total_files -eq 0 ]; then
#         warn_echo "没有需要复制的文件"
#         return 1
#     fi

#     echo ""
#     info_echo "开始自动分类复制... (${total_files} 个文件)"
#     echo ""
#     local progress_count=0
#     show_copy_progress 0 $total_files

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#             ((progress_count++))
#             show_copy_progress $progress_count $total_files
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#             ((progress_count++))
#             show_copy_progress $progress_count $total_files
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done
#     echo ""  # 进度条换行

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             v|V) do_view_car_shoot ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;
#     view-car-shoot) do_view_car_shoot ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac









# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了V功能)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_hidden "$cmd"; then
#         HIDDEN_COMMANDS=
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         DISABLED_COMMANDS=g
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS=g
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi
#     sed -i "s/DISABLED_COMMANDS=g
#     sed -i "s/HIDDEN_COMMANDS=
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按数字切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 复制进度条 =====================
# show_copy_progress() {
#     local current=$1
#     local total=$2
#     local width=30
#     if [ $total -eq 0 ]; then
#         printf "\r  [░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]   0%% (0/0)"
#         return
#     fi
#     local percent=$((current * 100 / total))
#     local filled=$((current * width / total))
#     local empty=$((width - filled))

#     printf "\r  ["
#     printf "%${filled}s" | tr ' ' '█'
#     printf "%${empty}s" | tr ' ' '░'
#     printf "] %3d%% (%d/%d)" "$percent" "$current" "$total"
# }

# # ===================== 查看 car_shoot 分类统计 =====================
# do_view_car_shoot() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "$target_dir" ]; then
#         warn_echo "car_shoot 目录不存在: ${target_dir}"
#         info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
#         return 1
#     fi

#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     if [ ! -d "$classify_dir" ]; then
#         warn_echo "分类目录不存在: ${classify_dir}"
#         info_echo "请先执行 [w] 自动分类复制到 Windows"
#         return 1
#     fi

#     local categories=("materials" "traffic" "weapon")
#     local total=0

#     for category in "${categories[@]}"; do
#         local dir="${classify_dir}/${category}"
#         local add_dir="${classify_dir}/add_${category}"
#         local count=0
#         local add_count=0

#         if [ -d "$dir" ]; then
#             count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
#         fi
#         if [ -d "$add_dir" ]; then
#             add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
#         fi

#         local subtotal=$((count + add_count))
#         total=$((total + subtotal))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $count -gt 0 ]; then
#             echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
#         fi
#         if [ $add_count -gt 0 ]; then
#             echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
#         fi
#         if [ $subtotal -gt 0 ]; then
#             echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
#         else
#             echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
#         fi
#         echo ""
#     done

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
#     echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 预扫描：统计本次需要复制的总文件数
#     local total_files=0
#     for category in "${categories[@]}"; do
#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             [[ "$basename" == add_* ]] && continue
#             ((total_files++))
#         done
#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             ((total_files++))
#         done
#     done

#     if [ $total_files -eq 0 ]; then
#         warn_echo "没有需要复制的文件"
#         return 1
#     fi

#     echo ""
#     info_echo "开始自动分类复制... (${total_files} 个文件)"
#     echo ""
#     local progress_count=0
#     show_copy_progress 0 $total_files

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#             ((progress_count++))
#             show_copy_progress $progress_count $total_files
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#             ((progress_count++))
#             show_copy_progress $progress_count $total_files
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done
#     echo ""  # 进度条换行

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             v|V) do_view_car_shoot ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;
#     view-car-shoot) do_view_car_shoot ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac








# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了V功能)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_hidden "$cmd"; then
#         HIDDEN_COMMANDS=
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         DISABLED_COMMANDS=g
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS=g
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi
#     sed -i "s/DISABLED_COMMANDS=g
#     sed -i "s/HIDDEN_COMMANDS=
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按数字切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 查看 car_shoot 分类统计 =====================
# do_view_car_shoot() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📂 car_shoot 分类统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "$target_dir" ]; then
#         warn_echo "car_shoot 目录不存在: ${target_dir}"
#         info_echo "请先创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot"
#         return 1
#     fi

#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     if [ ! -d "$classify_dir" ]; then
#         warn_echo "分类目录不存在: ${classify_dir}"
#         info_echo "请先执行 [w] 自动分类复制到 Windows"
#         return 1
#     fi

#     local categories=("materials" "traffic" "weapon")
#     local total=0

#     for category in "${categories[@]}"; do
#         local dir="${classify_dir}/${category}"
#         local add_dir="${classify_dir}/add_${category}"
#         local count=0
#         local add_count=0

#         if [ -d "$dir" ]; then
#             count=$(ls -1 "${dir}"/*.bmp 2>/dev/null | wc -l)
#         fi
#         if [ -d "$add_dir" ]; then
#             add_count=$(ls -1 "${add_dir}"/*.bmp 2>/dev/null | wc -l)
#         fi

#         local subtotal=$((count + add_count))
#         total=$((total + subtotal))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $count -gt 0 ]; then
#             echo -e "    常规: ${CLR_CYAN}${count}${CLR_RESET} 张"
#         fi
#         if [ $add_count -gt 0 ]; then
#             echo -e "    增补: ${CLR_CYAN}${add_count}${CLR_RESET} 张"
#         fi
#         if [ $subtotal -gt 0 ]; then
#             echo -e "    小计: ${CLR_BOLD}${subtotal}${CLR_RESET} 张"
#         else
#             echo -e "    ${CLR_YELLOW}(空)${CLR_RESET}"
#         fi
#         echo ""
#     done

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  ${CLR_BOLD}总计: ${total} 张${CLR_RESET}"
#     echo -e "  路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[v] 查看 car_shoot 分类统计${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,v,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             v|V) do_view_car_shoot ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;
#     view-car-shoot) do_view_car_shoot ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(添加了w下的进度条显示)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 统计总文件数（用于进度条）
#     local total_files=0
#     local current_file=0
#     for category in "${categories[@]}"; do
#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local bn=$(basename "$file")
#             [[ "$bn" == add_* ]] && continue
#             ((total_files++))
#         done
#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             ((total_files++))
#         done
#     done

#     if [ $total_files -gt 0 ]; then
#         info_echo "共 ${total_files} 张图片待复制，开始分类..."
#         echo ""
#     fi

#     # 执行复制（带进度条）
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#             ((current_file++))
#             # 显示进度条
#             if [ $total_files -gt 0 ]; then
#                 local pct=$((current_file * 100 / total_files))
#                 local filled=$((pct / 2))
#                 local empty=$((50 - filled))
#                 local bar=""
#                 for ((j=0; j<filled; j++)); do bar="${bar}█"; done
#                 for ((j=0; j<empty; j++)); do bar="${bar}░"; done
#                 printf "\r  ${CLR_CYAN}[%s] %3d%%${CLR_RESET}  %s → %s/%s" "$bar" "$pct" "$basename" "$category" "$(ls -1 ${dest_dir}/*.bmp 2>/dev/null | wc -l)"
#             fi
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#             ((current_file++))
#             # 显示进度条
#             if [ $total_files -gt 0 ]; then
#                 local pct=$((current_file * 100 / total_files))
#                 local filled=$((pct / 2))
#                 local empty=$((50 - filled))
#                 local bar=""
#                 for ((j=0; j<filled; j++)); do bar="${bar}█"; done
#                 for ((j=0; j<empty; j++)); do bar="${bar}░"; done
#                 printf "\r  ${CLR_CYAN}[%s] %3d%%${CLR_RESET}  %s → add_%s/%s" "$bar" "$pct" "$(basename $file)" "$category" "$(ls -1 ${add_dest_dir}/*.bmp 2>/dev/null | wc -l)"
#             fi
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 进度条完成，换行
#     if [ $total_files -gt 0 ]; then
#         echo ""
#         echo ""
#     fi

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) do_run_remote ;;
#             g|G) do_gen_win_bat ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 命令显示/禁用配置
# # DISABLED_COMMANDS: 禁用但显示（带🔒标记，选择时提示已禁用）
# # HIDDEN_COMMANDS: 完全隐藏（菜单里不显示该选项）
# # 格式: 用逗号分隔，如 "2,6" 或 "r,g"
# # 示例: DISABLED_COMMANDS=g
# #       表示[2][7]禁用显示🔒，[3][4][6]完全隐藏
# DISABLED_COMMANDS=g
# HIDDEN_COMMANDS=
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令状态管理 =====================
# check_disabled() {
#     local cmd="$1"
#     [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# check_hidden() {
#     local cmd="$1"
#     [[ ",${HIDDEN_COMMANDS}," == *",${cmd},"* ]] && return 0
#     return 1
# }

# cmd_status_text() {
#     local cmd="$1"
#     if check_hidden "$cmd"; then
#         echo -e "${CLR_BLUE}[已隐藏]${CLR_RESET}"
#     elif check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}"
#     else
#         echo -e "${CLR_GREEN}[已启用]${CLR_RESET}"
#     fi
# }

# toggle_command() {
#     local cmd="$1"
#     local name="$2"

#     if check_hidden "$cmd"; then
#         HIDDEN_COMMANDS=
#         info_echo "✅ [$cmd] $name 已启用"
#     elif check_disabled "$cmd"; then
#         DISABLED_COMMANDS=g
#         if [ -z "$HIDDEN_COMMANDS" ]; then HIDDEN_COMMANDS=
#         info_echo "👁️ [$cmd] $name 已隐藏（菜单不显示）"
#     else
#         if [ -z "$DISABLED_COMMANDS" ]; then DISABLED_COMMANDS=g
#         warn_echo "🔒 [$cmd] $name 已禁用（显示但不可选）"
#     fi

#     sed -i "s/DISABLED_COMMANDS=g
#     sed -i "s/HIDDEN_COMMANDS=
# }

# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令状态管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}状态说明:${CLR_RESET}"
#         echo -e "    ${CLR_GREEN}[已启用]${CLR_RESET}  = 正常显示，可用"
#         echo -e "    ${CLR_RED}[🔒 已禁用]${CLR_RESET} = 显示但带🔒，不可用"
#         echo -e "    ${CLR_BLUE}[已隐藏]${CLR_RESET}  = 菜单里不显示"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo -e "  当前隐藏: ${CLR_BLUE}${HIDDEN_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  按数字切换: ${CLR_GREEN}启用${CLR_RESET} → ${CLR_RED}禁用${CLR_RESET} → ${CLR_BLUE}隐藏${CLR_RESET} → ${CLR_GREEN}启用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(cmd_status_text 2)"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(cmd_status_text 3)"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(cmd_status_text 4)"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(cmd_status_text 6)"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(cmd_status_text 7)"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(cmd_status_text r)"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(cmd_status_text g)"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "
#         read -n1 -r dchoice
#         echo ""
#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac
#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         if ! check_hidden "2"; then echo -e "  [2] 全量部署（清理→编译→上传）                   $(cmd_status_text 2)"; fi
#         if ! check_hidden "3"; then echo -e "  [3] 仅编译（增量，不上传）                         $(cmd_status_text 3)"; fi
#         if ! check_hidden "4"; then echo -e "  [4] 仅上传（不编译）                               $(cmd_status_text 4)"; fi
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         if ! check_hidden "6"; then echo -e "  [6] 删除本地图片（image_shoot）                    $(cmd_status_text 6)"; fi
#         if ! check_hidden "7"; then echo -e "  [7] 删除远程图片（/tmp）                           $(cmd_status_text 7)"; fi
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         if ! check_hidden "r"; then echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project                  $(cmd_status_text r)${CLR_RESET}"; fi
#         if ! check_hidden "g"; then echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本               $(cmd_status_text g)${CLR_RESET}"; fi
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令状态管理（启用/禁用/隐藏）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_hidden "2"; then echo ""; warn_echo "[2] 已隐藏，无法选择"
#                 elif check_disabled "2"; then echo ""; warn_echo "🔒 [2] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_all; fi ;;
#             3) 
#                 if check_hidden "3"; then echo ""; warn_echo "[3] 已隐藏，无法选择"
#                 elif check_disabled "3"; then echo ""; warn_echo "🔒 [3] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_build; fi ;;
#             4) 
#                 if check_hidden "4"; then echo ""; warn_echo "[4] 已隐藏，无法选择"
#                 elif check_disabled "4"; then echo ""; warn_echo "🔒 [4] 已禁用"; info_echo "按 [d] 管理"
#                 else mode_upload; fi ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_hidden "6"; then echo ""; warn_echo "[6] 已隐藏，无法选择"
#                 elif check_disabled "6"; then echo ""; warn_echo "🔒 [6] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_bmp; fi ;;
#             7) 
#                 if check_hidden "7"; then echo ""; warn_echo "[7] 已隐藏，无法选择"
#                 elif check_disabled "7"; then echo ""; warn_echo "🔒 [7] 已禁用"; info_echo "按 [d] 管理"
#                 else do_clear_remote_bmp; fi ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_hidden "r"; then echo ""; warn_echo "[r] 已隐藏，无法选择"
#                 elif check_disabled "r"; then echo ""; warn_echo "🔒 [r] 已禁用"; info_echo "按 [d] 管理"
#                 else do_run_remote; fi ;;
#             g|G) 
#                 if check_hidden "g"; then echo ""; warn_echo "[g] 已隐藏，无法选择"
#                 elif check_disabled "g"; then echo ""; warn_echo "🔒 [g] 已禁用"; info_echo "按 [d] 管理"
#                 else do_gen_win_bat; fi ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"

# # 禁用命令配置（用逗号分隔，如 "2,6,7" 表示禁用全量部署、删除本地、删除远程）
# # 禁用的命令菜单上仍然显示，但带 🔒 标记，选择时提示已禁用
# DISABLED_COMMANDS=g
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 命令禁用管理 =====================
# # 检查命令是否被禁用
# check_disabled() {
#     local cmd="$1"
#     if [[ ",${DISABLED_COMMANDS}," == *",${cmd},"* ]]; then
#         return 0  # 已禁用
#     fi
#     return 1  # 未禁用
# }

# # 显示禁用标记
# show_disabled_mark() {
#     local cmd="$1"
#     if check_disabled "$cmd"; then
#         echo -e "${CLR_RED}[🔒]${CLR_RESET}"
#     fi
# }

# # 切换命令禁用状态
# toggle_command() {
#     local cmd="$1"
#     local name="$2"
#     if check_disabled "$cmd"; then
#         # 解除禁用
#         DISABLED_COMMANDS=g
#         info_echo "✅ 已启用 [$cmd] $name"
#     else
#         # 添加禁用
#         if [ -z "$DISABLED_COMMANDS" ]; then
#             DISABLED_COMMANDS=g
#         else
#             DISABLED_COMMANDS=g
#         fi
#         warn_echo "🔒 已禁用 [$cmd] $name"
#     fi

#     # 保存到脚本文件（永久生效）
#     sed -i "s/DISABLED_COMMANDS=g
# }

# # 管理禁用命令菜单
# manage_disabled() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ║         ⚙️  命令开关管理                 ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前禁用: ${CLR_RED}${DISABLED_COMMANDS:-无}${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1]${CLR_RESET} 全量部署（清理→编译→上传）       $(check_disabled 2 && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[2]${CLR_RESET} 仅编译（增量，不上传）          $(check_disabled 3 && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[3]${CLR_RESET} 仅上传（不编译）                $(check_disabled 4 && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[4]${CLR_RESET} 删除本地图片（image_shoot）     $(check_disabled 6 && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[5]${CLR_RESET} 删除远程图片（/tmp）            $(check_disabled 7 && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[6]${CLR_RESET} 远程运行 ./project              $(check_disabled r && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo -e "  ${CLR_GREEN}[7]${CLR_RESET} 生成 Windows 控制脚本           $(check_disabled g && echo -e "${CLR_RED}[🔒 已禁用]${CLR_RESET}" || echo -e "${CLR_GREEN}[已启用]${CLR_RESET}")"
#         echo ""
#         echo -e "  ${CLR_CYAN}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择要切换的命令 [0-7]: "

#         read -n1 -r dchoice
#         echo ""

#         case "$dchoice" in
#             1) toggle_command "2" "全量部署" ;;
#             2) toggle_command "3" "仅编译" ;;
#             3) toggle_command "4" "仅上传" ;;
#             4) toggle_command "6" "删除本地图片" ;;
#             5) toggle_command "7" "删除远程图片" ;;
#             6) toggle_command "r" "远程运行 ./project" ;;
#             7) toggle_command "g" "生成 Windows 控制脚本" ;;
#             0) return ;;
#             *) warn_echo "无效输入" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 远程运行 ./project =====================
# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）                   $(show_disabled_mark 2)"
#         echo -e "  [3] 仅编译（增量，不上传）                         $(show_disabled_mark 3)"
#         echo -e "  [4] 仅上传（不编译）                               $(show_disabled_mark 4)"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）                    $(show_disabled_mark 6)"
#         echo -e "  [7] 删除远程图片（/tmp）                           $(show_disabled_mark 7)"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project                  $(show_disabled_mark r)${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本               $(show_disabled_mark g)${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_CYAN}[d] ⚙️  命令开关管理（禁用/启用）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w,d]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) 
#                 if check_disabled "2"; then
#                     echo ""
#                     warn_echo "🔒 [2] 全量部署 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     mode_all
#                 fi
#                 ;;
#             3) 
#                 if check_disabled "3"; then
#                     echo ""
#                     warn_echo "🔒 [3] 仅编译 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     mode_build
#                 fi
#                 ;;
#             4) 
#                 if check_disabled "4"; then
#                     echo ""
#                     warn_echo "🔒 [4] 仅上传 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     mode_upload
#                 fi
#                 ;;
#             5) do_download_bmp ;;
#             6) 
#                 if check_disabled "6"; then
#                     echo ""
#                     warn_echo "🔒 [6] 删除本地图片 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     do_clear_bmp
#                 fi
#                 ;;
#             7) 
#                 if check_disabled "7"; then
#                     echo ""
#                     warn_echo "🔒 [7] 删除远程图片 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     do_clear_remote_bmp
#                 fi
#                 ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) 
#                 if check_disabled "r"; then
#                     echo ""
#                     warn_echo "🔒 [r] 远程运行 ./project 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     do_run_remote
#                 fi
#                 ;;
#             g|G) 
#                 if check_disabled "g"; then
#                     echo ""
#                     warn_echo "🔒 [g] 生成 Windows 控制脚本 已禁用"
#                     info_echo "按 [d] 进入命令开关管理解除禁用"
#                 else
#                     do_gen_win_bat
#                 fi
#                 ;;
#             d|D) manage_disabled ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac









# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 远程运行 ./project =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  远程运行 ./project${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  执行目录: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
#     echo ""

#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

#     local ret=$?
#     echo ""
#     if [ $ret -eq 0 ]; then
#         info_echo "✅ 远程项目执行完成"
#     else
#         warn_echo "⚠️  项目退出码: ${ret}"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. 项目未上传（先执行编译上传）"
#         echo "  2. 远程目录下没有 ./project 可执行文件"
#         echo "  3. 权限不足（可能需要 chmod +x project）"
#         echo "  4. 项目运行依赖缺失"
#     fi
# }

# # ===================== 生成 Windows 控制脚本 =====================
# do_gen_win_bat() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  生成 Windows 控制脚本${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ VMware 共享文件夹未挂载"
#         info_echo "请先执行: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << EOF
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@${vm_ip} "cd ${WORK_DIR} && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# EOF

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "✅ Windows 批处理脚本已生成"
#         echo ""
#         echo -e "${CLR_GREEN}文件位置:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}使用方法:${CLR_RESET}"
#         echo "  1. 切换到 Windows 桌面"
#         echo "  2. 双击 run_remote.bat"
#         echo "  3. 按 1 运行 build.sh 菜单（连接虚拟机）"
#         echo "  4. 按 2 运行 ./project（连接小车）"
#         echo ""
#         echo -e "${CLR_YELLOW}固定到任务栏:${CLR_RESET}"
#         echo "  右键 run_remote.bat → 固定到任务栏"
#         echo "  以后按 Win+数字 一键启动"
#         echo ""
#         echo -e "${CLR_YELLOW}注意:${CLR_RESET}"
#         echo "  Windows 需安装 OpenSSH 客户端"
#     else
#         warn_echo "❌ 生成失败"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[g] 生成 Windows 控制脚本（run_remote.bat）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,g,i,c,w]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) do_run_remote ;;
#             g|G) do_gen_win_bat ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     gen-win-bat)    do_gen_win_bat ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac








# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(windows系统操作拍摄数据集的第一版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# # ===================== 生成 Windows 远程控制脚本 =====================
# do_run_remote() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  Generate Windows Batch Script${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "VMware shared folder not mounted"
#         info_echo "Run: sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 获取虚拟机 IP
#     local vm_ip=$(hostname -I | awk '{print $1}')
#     if [ -z "$vm_ip" ]; then
#         vm_ip="192.168.229.131"
#     fi

#     # 确定 Windows 桌面路径
#     local win_desktop="/mnt/hgfs/Desktop"
#     if [ ! -d "$win_desktop" ]; then
#         win_desktop="/mnt/hgfs/car_shoot"
#     fi

#     # 生成 Windows 批处理文件（纯英文，避免编码问题）
#     local bat_file="${win_desktop}/run_remote.bat"

#     cat > "$bat_file" << 'BATEOF'
# @echo off
# chcp 65001 >nul
# title Remote Control Tool

# :MENU
# cls
# echo ========================================
# echo    Remote Control Tool
# echo ========================================
# echo.
# echo   [1] Run build.sh menu (Linux VM)
# echo   [2] Run ./project (Car 192.168.1.104)
# echo   [0] Exit
# echo.
# echo ========================================
# set /p choice="Select [0-2]: "

# if "%choice%"=="1" goto RUN_BUILD
# if "%choice%"=="2" goto RUN_PROJECT
# if "%choice%"=="0" goto EXIT
# if "%choice%"=="" goto MENU
# goto MENU

# :RUN_BUILD
# echo.
# echo Connecting to VM to run build.sh...
# echo.
# ssh -t kaka@VM_IP_PLACEHOLDER "cd /home/kaka/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user && ./build.sh"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :RUN_PROJECT
# echo.
# echo Connecting to Car to run ./project...
# echo.
# ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
# echo.
# echo Press any key to return...
# pause > nul
# goto MENU

# :EXIT
# exit
# BATEOF

#     # 替换虚拟机 IP 占位符
#     sed -i "s/VM_IP_PLACEHOLDER/${vm_ip}/g" "$bat_file"

#     # 转换换行符 LF -> CRLF（Windows批处理必需）
#     sed -i 's/$/\r/' "$bat_file"

#     if [ $? -eq 0 ]; then
#         info_echo "Windows batch script generated successfully"
#         echo ""
#         echo -e "${CLR_GREEN}File location:${CLR_RESET}"
#         echo -e "  ${CLR_CYAN}Windows Desktop\\run_remote.bat${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_YELLOW}Usage:${CLR_RESET}"
#         echo "  1. Go to Windows Desktop"
#         echo "  2. Double-click run_remote.bat"
#         echo "  3. Press 1 for build.sh menu (VM)"
#         echo "  4. Press 2 for ./project (Car)"
#         echo ""
#         echo -e "${CLR_YELLOW}Tip:${CLR_RESET}"
#         echo "  Right-click run_remote.bat -> Pin to taskbar"
#         echo "  Then press Win+number to launch quickly"
#         echo ""
#         echo -e "${CLR_YELLOW}Requirement:${CLR_RESET}"
#         echo "  Windows needs OpenSSH Client installed"
#         echo "  Settings -> Apps -> Optional features -> OpenSSH Client"
#     else
#         warn_echo "Failed to generate batch file"
#     fi
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[r] 生成 Windows 控制脚本（双击运行）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,r,i,c,w]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             r|R) do_run_remote ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     run-remote)     do_run_remote ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










#!/bin/bash
# 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)(比上一个版本的多了一个./project的功能)
#   ./build.sh           -> 弹出交互菜单
#   ./build.sh all       -> 全量部署（清理→编译→上传）
#   ./build.sh buildup   -> 增量编译+上传（日常最常用）
#   ./build.sh build     -> 仅编译（增量）
#   ./build.sh upload    -> 仅上传
#   ./build.sh sync-bmp  -> 下载并删除远程图片
#   ./build.sh count-bmp -> 统计本地图片数量
#   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# ===================== 核心配置区 =====================
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${WORK_DIR}/../out"
USER_DIR="${WORK_DIR}/../user"
REMOTE_IP="192.168.1.104"
REMOTE_USER="root"
REMOTE_PATH="/home/root/"
MAKE_JOBS=$(nproc)
RESERVE_FILE="本文件夹作用.txt"

# 图片相关配置（共用 REMOTE_IP）
LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
REMOTE_BMP_DIR="/tmp"

# Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# 自动分类目标子目录
CLASSIFY_BASE="fist_classify"
# =================================================================

# ===================== 颜色定义 =====================
CLR_RED='\033[31m'
CLR_GREEN='\033[32m'
CLR_YELLOW='\033[33m'
CLR_BLUE='\033[34m'
CLR_CYAN='\033[36m'
CLR_BOLD='\033[1m'
CLR_RESET='\033[0m'

# ===================== 通用函数 =====================
error_exit() {
    echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
    exit 1
}

info_echo() {
    echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
}

warn_echo() {
    echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
}

# ===================== 编译函数 =====================
do_cmake_make() {
    if [ ! -d "${OUT_DIR}" ]; then
        info_echo "创建编译目录: ${OUT_DIR}"
        mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
    fi

    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    info_echo "执行 cmake: ${USER_DIR}"
    cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

    info_echo "执行 make -j${MAKE_JOBS} ..."
    make -j${MAKE_JOBS} || error_exit "make 编译失败！"

    info_echo "✅ 编译成功"
}

# ===================== 上传函数 =====================
do_upload() {
    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    parent_dir_name=$(basename "$(dirname "$(pwd)")")
    info_echo "待上传: ${parent_dir_name}"

    if [ ! -e "${parent_dir_name}" ]; then
        error_exit "找不到产物: ${parent_dir_name}，请先编译！"
    fi

    info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
    scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
        || error_exit "上传失败，请检查网络/权限！"

    info_echo "✅ 上传完成"
}

# ===================== 全量部署 =====================
mode_all() {
    info_echo "【全量部署】清理 → 编译 → 上传"
    cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

    info_echo "清理目录，仅保留 ${RESERVE_FILE}"
    find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
        || error_exit "清理失败！"

    do_cmake_make
    do_upload

    info_echo "✅ 全量部署完成！"
}

# ===================== 增量编译+上传 =====================
mode_buildup() {
    info_echo "【增量编译+上传】不清理，编译后自动上传"
    do_cmake_make
    do_upload
    info_echo "✅ 增量编译并上传完成！"
}

# ===================== 仅编译 =====================
mode_build() {
    info_echo "【仅编译】增量编译，不上传"
    do_cmake_make
    info_echo "✅ 增量编译完成！"
}

# ===================== 仅上传 =====================
mode_upload() {
    info_echo "【仅上传】直接 scp 已有产物"
    do_upload
    info_echo "✅ 上传完成！"
}

# ===================== 图片操作函数 =====================
do_count_bmp() {
    if [ ! -d "${LOCAL_BMP_DIR}" ]; then
        echo "0"
        return 1
    fi
    local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
    echo "${count}"
}

show_count_bmp() {
    local local_count=$(do_count_bmp)
    info_echo "本地图片数量: ${local_count} 张"
    info_echo "目录: ${LOCAL_BMP_DIR}"

    # 统计远程图片数量
    local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
    if [ $? -eq 0 ]; then
        info_echo "远程图片数量: ${remote_count} 张"
        info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
    else
        warn_echo "远程统计失败，请检查连接"
    fi
}

do_download_bmp() {
    info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
    if [ ! -d "${LOCAL_BMP_DIR}" ]; then
        mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
    fi
    scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
    if [ $? -eq 0 ]; then
        info_echo "✅ 下载完成"
        show_count_bmp
    else
        warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
        return 1
    fi
}

do_clear_bmp() {
    info_echo "正在删除本地 *.bmp ..."
    find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
    info_echo "✅ 本地图片已清空"
    show_count_bmp
}

do_clear_remote_bmp() {
    info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
    if [ $? -eq 0 ]; then
        info_echo "✅ 远程图片已删除"
    else
        warn_echo "远程删除失败"
        return 1
    fi
}

do_sync_bmp_auto() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
    echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
    echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    do_download_bmp || return 1

    info_echo "正在删除远程图片..."
    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
    if [ $? -eq 0 ]; then
        info_echo "✅ 远程图片已删除"
    else
        warn_echo "远程删除可能失败"
    fi

    local count=$(do_count_bmp)
    echo ""
    info_echo "📊 当前本地图片数量: ${count} 张"
    info_echo "🌐 远程IP: ${REMOTE_IP}"
}

do_ping_bmp() {
    info_echo "测试连接 ${REMOTE_IP} ..."
    ping -c 3 "${REMOTE_IP}"
}

do_run_remote() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  🚀 远程运行项目${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  目标设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
    echo -e "  执行命令: ${CLR_CYAN}./project${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    info_echo "正在通过 SSH 在远程设备上执行 ./project ..."
    echo ""

    # 在远程设备上执行：先进入上传目录，然后运行 ./project
    ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_PATH} && ./project"

    local ret=$?
    echo ""
    if [ $ret -eq 0 ]; then
        info_echo "✅ 远程项目执行完成"
    else
        warn_echo "⚠️  项目退出码: ${ret}"
        echo ""
        echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
        echo "  1. 项目未上传（先执行编译上传）"
        echo "  2. 远程目录下没有 ./project 可执行文件"
        echo "  3. 权限不足（可能需要 chmod +x project）"
        echo "  4. 项目运行依赖缺失"
    fi
}


set_remote_ip() {
    echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
    read -r new_ip
    if [ -n "$new_ip" ]; then
        sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
        REMOTE_IP="$new_ip"
        info_echo "✅ 已更新远程IP为: ${new_ip}"
        info_echo "（重新运行脚本生效）"
    else
        warn_echo "IP 未修改"
    fi
}

show_bmp_config() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
    echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
    echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
    echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
    echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
    echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
    echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
    echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
    echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
    echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
}

# ===================== 自动分类复制到 Windows =====================
do_copy_to_windows() {
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    # 检测共享文件夹挂载点
    if [ ! -d "/mnt/hgfs" ]; then
        warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
        echo ""
        echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
        echo "  sudo mkdir -p /mnt/hgfs"
        echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
        return 1
    fi

    # 检测目标目录
    local target_dir="${WINDOWS_SHARED_DIR}"
    if [ ! -d "${target_dir}" ]; then
        warn_echo "目标目录不存在: ${target_dir}"
        info_echo "尝试创建..."
        mkdir -p "${target_dir}" 2>/dev/null
        if [ $? -ne 0 ]; then
            warn_echo "无法创建，尝试查找可用的共享文件夹..."
            echo ""
            echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
            ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
            echo ""
            echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
            read -r target_dir
            if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
                warn_echo "路径无效，取消操作"
                return 1
            fi
        fi
    fi

    # 先确保本地有图片（如果本地没有，先从远程下载）
    local local_count=$(do_count_bmp)
    if [ "${local_count}" -eq 0 ]; then
        info_echo "本地没有图片，先从远程下载..."
        do_download_bmp || return 1
    fi

    # 分类目标根目录
    local classify_dir="${target_dir}/${CLASSIFY_BASE}"
    mkdir -p "${classify_dir}" 2>/dev/null

    echo ""
    info_echo "开始自动分类复制..."
    info_echo "源: ${LOCAL_BMP_DIR}"
    info_echo "目标: ${classify_dir}"
    echo ""

    # 定义分类规则
    local categories=("materials" "traffic" "weapon")
    local total_copied=0
    local total_add_copied=0

    # 用于存储统计信息
    declare -A old_counts
    declare -A new_counts
    declare -A add_old_counts
    declare -A add_new_counts

    # 先统计各类别复制前的数量
    for category in "${categories[@]}"; do
        local dest_dir="${classify_dir}/${category}"
        local add_dest_dir="${classify_dir}/add_${category}"
        mkdir -p "${dest_dir}" "${add_dest_dir}"
        old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
        add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
    done

    # 执行复制
    for category in "${categories[@]}"; do
        local count=0
        local add_count=0

        # 旧格式: category_*.bmp（排除 add_ 前缀）
        local dest_dir="${classify_dir}/${category}"

        for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
            [ -e "$file" ] || continue
            local basename=$(basename "$file")
            # 跳过 add_ 前缀的文件
            if [[ "$basename" == add_* ]]; then
                continue
            fi
            cp "$file" "${dest_dir}/"
            ((count++))
        done

        # 增补格式: add_category_*.bmp
        local add_dest_dir="${classify_dir}/add_${category}"

        for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
            [ -e "$file" ] || continue
            cp "$file" "${add_dest_dir}/"
            ((add_count++))
        done

        # 统计复制后的数量
        new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
        add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

        total_copied=$((total_copied + count))
        total_add_copied=$((total_add_copied + add_count))
    done

    # 统计未分类的文件
    local uncategorized=0
    for file in "${LOCAL_BMP_DIR}"/*.bmp; do
        [ -e "$file" ] || continue
        local basename=$(basename "$file")
        local classified=false
        for category in "${categories[@]}"; do
            if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
                classified=true
                break
            fi
        done
        if [ "$classified" = false ]; then
            ((uncategorized++))
        fi
    done

    # 显示详细统计
    echo ""
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""

    for category in "${categories[@]}"; do
        local old=${old_counts[$category]}
        local new=${new_counts[$category]}
        local added=$((new - old))
        local add_old=${add_old_counts[$category]}
        local add_new=${add_new_counts[$category]}
        local add_added=$((add_new - add_old))

        echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
        if [ $added -gt 0 ]; then
            echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
        elif [ $new -gt 0 ]; then
            echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
        fi

        if [ $add_added -gt 0 ]; then
            echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
        elif [ $add_new -gt 0 ]; then
            echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
        fi
        echo ""
    done

    if [ $uncategorized -gt 0 ]; then
        echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
        echo ""
    fi

    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
    if [ $total_add_copied -gt 0 ]; then
        echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
    fi
    echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
    echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
    echo ""
    info_echo "✅ 复制完成！Windows 桌面可直接查看"

    # 询问是否删除本地图片
    echo -n "  是否删除本地已复制的图片？[y/N]: "
    read -n1 -r confirm
    echo ""
    if [[ "$confirm" == [yY] ]]; then
        for category in "${categories[@]}"; do
            rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
            rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
        done
        info_echo "✅ 本地图片已清理"
        show_count_bmp
    fi
}

# ===================== 交互主菜单（全部展开，无子菜单） =====================
show_menu() {
    while true; do
        echo ""
        echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
        echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
        echo ""
        echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
        echo ""
        echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
        echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
        echo -e "  [2] 全量部署（清理→编译→上传）"
        echo -e "  [3] 仅编译（增量，不上传）"
        echo -e "  [4] 仅上传（不编译）"
        echo ""
        echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
        echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
        echo -e "  [6] 删除本地图片（image_shoot）"
        echo -e "  [7] 删除远程图片（/tmp）"
        echo -e "  [8] 查看本地+远程图片数量"
        echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
        echo ""
        echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
        echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
        echo -e "  ${CLR_YELLOW}[r] 远程运行 ./project${CLR_RESET}"
        echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
        echo -e "  [c] 查看当前配置"
        echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
        echo ""
        echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
        echo ""
        echo -n "  请选择操作 [0-9,p,r,i,c,w]: "

        read -n1 -r choice
        echo ""

        case "$choice" in
            1) mode_buildup ;;
            2) mode_all ;;
            3) mode_build ;;
            4) mode_upload ;;
            5) do_download_bmp ;;
            6) do_clear_bmp ;;
            7) do_clear_remote_bmp ;;
            8) show_count_bmp ;;
            9) do_sync_bmp_auto ;;
            p|P) do_ping_bmp ;;
            r|R) do_run_remote ;;
            i|I) set_remote_ip ;;
            c|C) show_bmp_config ;;
            w|W) do_copy_to_windows ;;
            0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
            *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
        esac

        echo ""
        echo -n "  按任意键继续..."
        read -n1 -r
        echo ""
    done
}

# ===================== 入口：参数解析 =====================
MODE="${1:-menu}"

case "$MODE" in
    all)        mode_all ;;
    buildup|bu) mode_buildup ;;
    build)      mode_build ;;
    upload|up)  mode_upload ;;

    # 图片相关命令
    download-bmp)   do_download_bmp ;;
    clear-bmp)      do_clear_bmp ;;
    clear-remote-bmp) do_clear_remote_bmp ;;
    count-bmp)      show_count_bmp ;;
    sync-bmp-auto)  do_sync_bmp_auto ;;
    sync-bmp)       do_sync_bmp_auto ;;
    ping-bmp)       do_ping_bmp ;;
    run-remote)     do_run_remote ;;
    copy-win)       do_copy_to_windows ;;

    # 其他
    config)         show_bmp_config ;;
    set-ip)         set_remote_ip ;;

    menu|"")    show_menu ;;
    *)
        echo ""
        echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
        echo ""
        echo "编译部署:"
        echo "  ./build.sh [all|buildup|build|upload]"
        echo ""
        echo "图片工具（Linux）:"
        echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
        echo "  ./build.sh clear-bmp         删除本地图片"
        echo "  ./build.sh clear-remote-bmp  删除远程图片"
        echo "  ./build.sh count-bmp         查看本地图片数量"
        echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
        echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
        echo ""
        echo "图片工具（Windows）:"
        echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
        echo ""
        echo "其他:"
        echo "  ./build.sh ping-bmp          测试远程连接"
        echo "  ./build.sh config            查看当前配置"
        echo "  ./build.sh set-ip            修改远程IP"
        echo "  ./build.sh                   启动交互菜单"
        echo ""
        exit 1
        ;;
esac









# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 自动分类复制到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置（VMware 默认挂载 /mnt/hgfs/）
# # 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 自动分类目标子目录
# CLASSIFY_BASE="fist_classify"
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local local_count=$(do_count_bmp)
#     info_echo "本地图片数量: ${local_count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"

#     # 统计远程图片数量
#     local remote_count=$(ssh "${REMOTE_USER}@${REMOTE_IP}" "ls -1 ${REMOTE_BMP_DIR}/*.bmp 2>/dev/null | wc -l")
#     if [ $? -eq 0 ]; then
#         info_echo "远程图片数量: ${remote_count} 张"
#         info_echo "远程目录: ${REMOTE_IP}:${REMOTE_BMP_DIR}"
#     else
#         warn_echo "远程统计失败，请检查连接"
#     fi
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "  分类目录:   ${CLR_CYAN}${CLASSIFY_BASE}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 自动分类复制到 Windows =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  自动分类复制到 Windows${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 分类目标根目录
#     local classify_dir="${target_dir}/${CLASSIFY_BASE}"
#     mkdir -p "${classify_dir}" 2>/dev/null

#     echo ""
#     info_echo "开始自动分类复制..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${classify_dir}"
#     echo ""

#     # 定义分类规则
#     local categories=("materials" "traffic" "weapon")
#     local total_copied=0
#     local total_add_copied=0

#     # 用于存储统计信息
#     declare -A old_counts
#     declare -A new_counts
#     declare -A add_old_counts
#     declare -A add_new_counts

#     # 先统计各类别复制前的数量
#     for category in "${categories[@]}"; do
#         local dest_dir="${classify_dir}/${category}"
#         local add_dest_dir="${classify_dir}/add_${category}"
#         mkdir -p "${dest_dir}" "${add_dest_dir}"
#         old_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_old_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)
#     done

#     # 执行复制
#     for category in "${categories[@]}"; do
#         local count=0
#         local add_count=0

#         # 旧格式: category_*.bmp（排除 add_ 前缀）
#         local dest_dir="${classify_dir}/${category}"

#         for file in "${LOCAL_BMP_DIR}"/${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             local basename=$(basename "$file")
#             # 跳过 add_ 前缀的文件
#             if [[ "$basename" == add_* ]]; then
#                 continue
#             fi
#             cp "$file" "${dest_dir}/"
#             ((count++))
#         done

#         # 增补格式: add_category_*.bmp
#         local add_dest_dir="${classify_dir}/add_${category}"

#         for file in "${LOCAL_BMP_DIR}"/add_${category}_*.bmp; do
#             [ -e "$file" ] || continue
#             cp "$file" "${add_dest_dir}/"
#             ((add_count++))
#         done

#         # 统计复制后的数量
#         new_counts[$category]=$(ls -1 "${dest_dir}"/*.bmp 2>/dev/null | wc -l)
#         add_new_counts[$category]=$(ls -1 "${add_dest_dir}"/*.bmp 2>/dev/null | wc -l)

#         total_copied=$((total_copied + count))
#         total_add_copied=$((total_add_copied + add_count))
#     done

#     # 统计未分类的文件
#     local uncategorized=0
#     for file in "${LOCAL_BMP_DIR}"/*.bmp; do
#         [ -e "$file" ] || continue
#         local basename=$(basename "$file")
#         local classified=false
#         for category in "${categories[@]}"; do
#             if [[ "$basename" == ${category}_* ]] || [[ "$basename" == add_${category}_* ]]; then
#                 classified=true
#                 break
#             fi
#         done
#         if [ "$classified" = false ]; then
#             ((uncategorized++))
#         fi
#     done

#     # 显示详细统计
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📊 分类复制统计${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     for category in "${categories[@]}"; do
#         local old=${old_counts[$category]}
#         local new=${new_counts[$category]}
#         local added=$((new - old))
#         local add_old=${add_old_counts[$category]}
#         local add_new=${add_new_counts[$category]}
#         local add_added=$((add_new - add_old))

#         echo -e "  ${CLR_GREEN}[${category}]${CLR_RESET}"
#         if [ $added -gt 0 ]; then
#             echo -e "    常规: 原有 ${CLR_YELLOW}${old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${new}${CLR_RESET} 张)"
#         elif [ $new -gt 0 ]; then
#             echo -e "    常规: 总计 ${CLR_CYAN}${new}${CLR_RESET} 张 (无新增)"
#         fi

#         if [ $add_added -gt 0 ]; then
#             echo -e "    增补: 原有 ${CLR_YELLOW}${add_old}${CLR_RESET} 张 → 新增 ${CLR_GREEN}${add_added}${CLR_RESET} 张 (总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张)"
#         elif [ $add_new -gt 0 ]; then
#             echo -e "    增补: 总计 ${CLR_CYAN}${add_new}${CLR_RESET} 张 (无新增)"
#         fi
#         echo ""
#     done

#     if [ $uncategorized -gt 0 ]; then
#         echo -e "  ${CLR_YELLOW}[未分类] ${uncategorized} 张（未匹配前缀）${CLR_RESET}"
#         echo ""
#     fi

#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  本次新增常规: ${CLR_GREEN}${total_copied} 张${CLR_RESET}"
#     if [ $total_add_copied -gt 0 ]; then
#         echo -e "  本次新增增补: ${CLR_CYAN}${total_add_copied} 张${CLR_RESET}"
#     fi
#     echo -e "  目标路径: ${CLR_CYAN}${classify_dir}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""
#     info_echo "✅ 复制完成！Windows 桌面可直接查看"

#     # 询问是否删除本地图片
#     echo -n "  是否删除本地已复制的图片？[y/N]: "
#     read -n1 -r confirm
#     echo ""
#     if [[ "$confirm" == [yY] ]]; then
#         for category in "${categories[@]}"; do
#             rm -f "${LOCAL_BMP_DIR}"/${category}_*.bmp
#             rm -f "${LOCAL_BMP_DIR}"/add_${category}_*.bmp
#         done
#         info_echo "✅ 本地图片已清理"
#         show_count_bmp
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片（image_shoot）"
#         echo -e "  [7] 删除远程图片（/tmp）"
#         echo -e "  [8] 查看本地+远程图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片（/tmp-->image_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 自动分类复制到 Windows（共享文件夹car_shoot） ★${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,i,c,w]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          自动分类复制到 Windows（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# #   ./build.sh copy-win  -> 复制图片到 Windows（VMware共享文件夹）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # Windows 共享文件夹配置
# # VMware 共享文件夹名称是 car_shoot，挂载后在 /mnt/hgfs/car_shoot/
# # 建议创建软链接: ln -s /mnt/hgfs/car_shoot car_shoot
# WINDOWS_SHARED_DIR="${WORK_DIR}/car_shoot"
# # 如果还没创建软链接，也可以直接写:
# # WINDOWS_SHARED_DIR="/mnt/hgfs/car_shoot"
# # =================================================================

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local count=$(do_count_bmp)
#     info_echo "本地图片数量: ${count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "  Win共享:    ${CLR_CYAN}${WINDOWS_SHARED_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 复制到 Windows（VMware共享文件夹） =====================
# do_copy_to_windows() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  🖥️  复制图片到 Windows 桌面${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     # 检测共享文件夹挂载点
#     if [ ! -d "/mnt/hgfs" ]; then
#         warn_echo "❌ 未检测到 VMware 共享文件夹挂载点 /mnt/hgfs/"
#         echo ""
#         echo -e "${CLR_YELLOW}修复方法：${CLR_RESET}"
#         echo "  sudo mkdir -p /mnt/hgfs"
#         echo "  sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other -o uid=1000"
#         echo ""
#         echo -e "${CLR_YELLOW}永久挂载（写入fstab）：${CLR_RESET}"
#         echo "  echo '.host:/ /mnt/hgfs fuse.vmhgfs-fuse allow_other,uid=1000,defaults 0 0' | sudo tee -a /etc/fstab"
#         return 1
#     fi

#     # 检测目标目录
#     local target_dir="${WINDOWS_SHARED_DIR}"
#     if [ ! -d "${target_dir}" ]; then
#         warn_echo "目标目录不存在: ${target_dir}"
#         info_echo "尝试创建..."
#         mkdir -p "${target_dir}" 2>/dev/null
#         if [ $? -ne 0 ]; then
#             warn_echo "无法创建，尝试查找可用的共享文件夹..."
#             echo ""
#             echo -e "${CLR_YELLOW}当前 /mnt/hgfs/ 下的内容：${CLR_RESET}"
#             ls -la /mnt/hgfs/ 2>/dev/null || echo "（无内容）"
#             echo ""
#             echo -n "请手动输入共享文件夹路径（如 /mnt/hgfs/car_shoot）: "
#             read -r target_dir
#             if [ -z "${target_dir}" ] || [ ! -d "${target_dir}" ]; then
#                 warn_echo "路径无效，取消操作"
#                 return 1
#             fi
#         fi
#     fi

#     # 先确保本地有图片（如果本地没有，先从远程下载）
#     local local_count=$(do_count_bmp)
#     if [ "${local_count}" -eq 0 ]; then
#         info_echo "本地没有图片，先从远程下载..."
#         do_download_bmp || return 1
#     fi

#     # 复制到 Windows
#     info_echo "正在复制图片到 Windows 共享目录..."
#     info_echo "源: ${LOCAL_BMP_DIR}"
#     info_echo "目标: ${target_dir}"
#     echo ""

#     cp "${LOCAL_BMP_DIR}"/*.bmp "${target_dir}/" 2>/dev/null

#     if [ $? -eq 0 ]; then
#         local win_count=$(ls -1 "${target_dir}"/*.bmp 2>/dev/null | wc -l)
#         echo ""
#         info_echo "✅ 复制完成！"
#         info_echo "📊 Windows 目录现有 ${win_count} 张图片"
#         info_echo "📂 路径: ${target_dir}"
#         echo ""
#         info_echo "（Windows 桌面可直接看到）"
#     else
#         warn_echo "❌ 复制失败"
#         echo ""
#         echo -e "${CLR_YELLOW}可能原因：${CLR_RESET}"
#         echo "  1. VMware 共享文件夹未开启"
#         echo "  2. 目标路径不存在"
#         echo "  3. 权限不足"
#         return 1
#     fi
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地（Linux）${CLR_RESET}"
#         echo -e "  [6] 删除本地图片"
#         echo -e "  [7] 删除远程图片"
#         echo -e "  [8] 查看本地图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片 ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置/Windows】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo -e "  ${CLR_YELLOW}[w] 复制图片到 Windows 桌面${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,i,c,w]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             w|W) do_copy_to_windows ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;
#     copy-win)       do_copy_to_windows ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具（Linux）:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地（Linux虚拟机）"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo ""
#         echo "图片工具（Windows）:"
#         echo "  ./build.sh copy-win          复制图片到 Windows 桌面（VMware共享文件夹）"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，无子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local count=$(do_count_bmp)
#     info_echo "本地图片数量: ${count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 交互主菜单（全部展开，无子菜单） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 综合工具菜单         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}  |  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载远程图片到本地${CLR_RESET}"
#         echo -e "  [6] 删除本地图片"
#         echo -e "  [7] 删除远程图片"
#         echo -e "  [8] 查看本地图片数量"
#         echo -e "  ${CLR_BLUE}[9] 下载并删除远程图片 ★${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_YELLOW}  【网络/配置】${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[p] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[i] 修改远程IP${CLR_RESET}"
#         echo -e "  [c] 查看当前配置"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-9,p,i,c]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) do_download_bmp ;;
#             6) do_clear_bmp ;;
#             7) do_clear_remote_bmp ;;
#             8) show_count_bmp ;;
#             9) do_sync_bmp_auto ;;
#             p|P) do_ping_bmp ;;
#             i|I) set_remote_ip ;;
#             c|C) show_bmp_config ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 综合工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单(编译+图片管理，有子菜单版)
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# #   ./build.sh sync-bmp  -> 下载并删除远程图片
# #   ./build.sh count-bmp -> 统计本地图片数量
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # 图片相关配置（共用 REMOTE_IP）
# LOCAL_BMP_DIR="${HOME}/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"
# REMOTE_BMP_DIR="/tmp"

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 图片操作函数 =====================
# do_count_bmp() {
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         echo "0"
#         return 1
#     fi
#     local count=$(find "${LOCAL_BMP_DIR}" -maxdepth 1 -name "*.bmp" | wc -l)
#     echo "${count}"
# }

# show_count_bmp() {
#     local count=$(do_count_bmp)
#     info_echo "本地图片数量: ${count} 张"
#     info_echo "目录: ${LOCAL_BMP_DIR}"
# }

# do_download_bmp() {
#     info_echo "正在从 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下载 *.bmp ..."
#     if [ ! -d "${LOCAL_BMP_DIR}" ]; then
#         mkdir -p "${LOCAL_BMP_DIR}" || error_exit "无法创建本地目录: ${LOCAL_BMP_DIR}"
#     fi
#     scp -O "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_BMP_DIR}/*.bmp" "${LOCAL_BMP_DIR}/"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 下载完成"
#         show_count_bmp
#     else
#         warn_echo "下载失败，请检查 IP (${REMOTE_IP}) 或远程目录"
#         return 1
#     fi
# }

# do_clear_bmp() {
#     info_echo "正在删除本地 *.bmp ..."
#     find "${LOCAL_BMP_DIR}" -name "*.bmp" -type f -delete
#     info_echo "✅ 本地图片已清空"
#     show_count_bmp
# }

# do_clear_remote_bmp() {
#     info_echo "正在删除远程 ${REMOTE_IP}:${REMOTE_BMP_DIR} 下的 *.bmp ..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除失败"
#         return 1
#     fi
# }

# do_sync_bmp_auto() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  📷 图片同步工具${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程目录: ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地目录: ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo ""

#     do_download_bmp || return 1

#     info_echo "正在删除远程图片..."
#     ssh "${REMOTE_USER}@${REMOTE_IP}" "cd ${REMOTE_BMP_DIR} && rm -rf *.bmp"
#     if [ $? -eq 0 ]; then
#         info_echo "✅ 远程图片已删除"
#     else
#         warn_echo "远程删除可能失败"
#     fi

#     local count=$(do_count_bmp)
#     echo ""
#     info_echo "📊 当前本地图片数量: ${count} 张"
#     info_echo "🌐 远程IP: ${REMOTE_IP}"
# }

# do_ping_bmp() {
#     info_echo "测试连接 ${REMOTE_IP} ..."
#     ping -c 3 "${REMOTE_IP}"
# }

# set_remote_ip() {
#     echo -n "当前IP: ${REMOTE_IP}，请输入新IP: "
#     read -r new_ip
#     if [ -n "$new_ip" ]; then
#         sed -i "s/REMOTE_IP=\"${REMOTE_IP}\"/REMOTE_IP=\"${new_ip}\"/" "${BASH_SOURCE[0]}"
#         REMOTE_IP="$new_ip"
#         info_echo "✅ 已更新远程IP为: ${new_ip}"
#         info_echo "（重新运行脚本生效）"
#     else
#         warn_echo "IP 未修改"
#     fi
# }

# show_bmp_config() {
#     echo ""
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "${CLR_BOLD}  ⚙️  当前配置${CLR_RESET}"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
#     echo -e "  远程IP:     ${CLR_CYAN}${REMOTE_IP}${CLR_RESET}"
#     echo -e "  远程用户:   ${CLR_CYAN}${REMOTE_USER}${CLR_RESET}"
#     echo -e "  上传路径:   ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#     echo -e "  远程图片:   ${CLR_CYAN}${REMOTE_BMP_DIR}${CLR_RESET}"
#     echo -e "  本地图片:   ${CLR_CYAN}${LOCAL_BMP_DIR}${CLR_RESET}"
#     echo -e "  编译线程:   ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#     echo -e "  输出目录:   ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#     echo -e "  本地图片数: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#     echo -e "${CLR_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${CLR_RESET}"
# }

# # ===================== 图片子菜单（免回车版，无clear） =====================
# show_bmp_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║           📷 图片操作工具菜单           ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前远程: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#         echo -e "  本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "  ${CLR_GREEN}[1] 下载远程图片到本地${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[2] 删除本地图片${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[3] 删除远程图片${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[4] 查看本地图片数量${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 下载并删除远程图片 ★ 最常用${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}[6] 测试远程连接（ping）${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[7] 修改远程IP${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 返回主菜单${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-7]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) do_download_bmp ;;
#             2) do_clear_bmp ;;
#             3) do_clear_remote_bmp ;;
#             4) show_count_bmp ;;
#             5) do_sync_bmp_auto ;;
#             6) do_ping_bmp ;;
#             7) set_remote_ip ;;
#             0) return ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 交互主菜单（免回车版，无clear） =====================
# show_menu() {
#     while true; do
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 编译部署工具         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置:"
#         echo -e "    远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#         echo -e "    上传路径: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#         echo -e "    编译线程: ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#         echo -e "    输出目录: ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#         echo -e "    本地图片: ${CLR_CYAN}$(do_count_bmp)${CLR_RESET} 张"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  【编译部署】${CLR_RESET}"
#         echo -e "  ${CLR_GREEN}[1] 增量编译 + 上传  ★ 日常最常用${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_BLUE}  【图片工具】${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[5] 图片操作工具（下载/删除/同步）${CLR_RESET}"
#         echo -e "  ${CLR_BLUE}[6] 一键同步图片（下载+删除远程）${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_YELLOW}[7] 查看当前配置${CLR_RESET}"
#         echo -e "  ${CLR_YELLOW}[8] 修改远程IP${CLR_RESET}"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-8]: "

#         read -n1 -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             5) show_bmp_menu ;;
#             6) do_sync_bmp_auto ;;
#             7) show_bmp_config ;;
#             8) set_remote_ip ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，按任意键继续..."; read -n1 -r; echo "" ;;
#         esac

#         echo ""
#         echo -n "  按任意键继续..."
#         read -n1 -r
#         echo ""
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;

#     # 图片相关命令
#     download-bmp)   do_download_bmp ;;
#     clear-bmp)      do_clear_bmp ;;
#     clear-remote-bmp) do_clear_remote_bmp ;;
#     count-bmp)      show_count_bmp ;;
#     sync-bmp-auto)  do_sync_bmp_auto ;;
#     sync-bmp)       do_sync_bmp_auto ;;
#     ping-bmp)       do_ping_bmp ;;

#     # 其他
#     config)         show_bmp_config ;;
#     set-ip)         set_remote_ip ;;

#     menu|"")    show_menu ;;
#     *)
#         echo ""
#         echo -e "${CLR_BOLD}龙芯 2K0300 编译部署工具${CLR_RESET}"
#         echo ""
#         echo "编译部署:"
#         echo "  ./build.sh [all|buildup|build|upload]"
#         echo ""
#         echo "图片工具:"
#         echo "  ./build.sh download-bmp      下载远程图片到本地"
#         echo "  ./build.sh clear-bmp         删除本地图片"
#         echo "  ./build.sh clear-remote-bmp  删除远程图片"
#         echo "  ./build.sh count-bmp         查看本地图片数量"
#         echo "  ./build.sh sync-bmp-auto     下载并删除远程图片（最常用）"
#         echo "  ./build.sh sync-bmp          sync-bmp-auto 的简写"
#         echo "  ./build.sh ping-bmp          测试远程连接"
#         echo ""
#         echo "其他:"
#         echo "  ./build.sh config            查看当前配置"
#         echo "  ./build.sh set-ip            修改远程IP"
#         echo "  ./build.sh                   启动交互菜单"
#         echo ""
#         exit 1
#         ;;
# esac










# #!/bin/bash
# # 交互式菜单脚本：支持命令行参数直接执行，无参数时弹出菜单
# #   ./build.sh           -> 弹出交互菜单
# #   ./build.sh all       -> 全量部署（清理→编译→上传）
# #   ./build.sh buildup   -> 增量编译+上传（日常最常用）
# #   ./build.sh build     -> 仅编译（增量）
# #   ./build.sh upload    -> 仅上传
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # ===================== 颜色定义 =====================
# CLR_RED='\033[31m'
# CLR_GREEN='\033[32m'
# CLR_YELLOW='\033[33m'
# CLR_BLUE='\033[34m'
# CLR_CYAN='\033[36m'
# CLR_BOLD='\033[1m'
# CLR_RESET='\033[0m'

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "${CLR_RED}[ERROR] $1${CLR_RESET}"
#     exit 1
# }

# info_echo() {
#     echo -e "${CLR_GREEN}[INFO] $1${CLR_RESET}"
# }

# warn_echo() {
#     echo -e "${CLR_YELLOW}[WARN] $1${CLR_RESET}"
# }

# # ===================== 编译函数 =====================
# do_cmake_make() {
#     if [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 失败，请检查 CMakeLists.txt 或依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数 =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传: ${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到产物: ${parent_dir_name}，请先编译！"
#     fi

#     info_echo "上传至 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "上传失败，请检查网络/权限！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 全量部署 =====================
# mode_all() {
#     info_echo "【全量部署】清理 → 编译 → 上传"
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "清理目录，仅保留 ${RESERVE_FILE}"
#     find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#         || error_exit "清理失败！"

#     do_cmake_make
#     do_upload

#     info_echo "✅ 全量部署完成！"
# }

# # ===================== 增量编译+上传 =====================
# mode_buildup() {
#     info_echo "【增量编译+上传】不清理，编译后自动上传"
#     do_cmake_make
#     do_upload
#     info_echo "✅ 增量编译并上传完成！"
# }

# # ===================== 仅编译 =====================
# mode_build() {
#     info_echo "【仅编译】增量编译，不上传"
#     do_cmake_make
#     info_echo "✅ 增量编译完成！"
# }

# # ===================== 仅上传 =====================
# mode_upload() {
#     info_echo "【仅上传】直接 scp 已有产物"
#     do_upload
#     info_echo "✅ 上传完成！"
# }

# # ===================== 交互菜单 =====================
# show_menu() {
#     while true; do
#         clear
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╔══════════════════════════════════════════╗${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ║         龙芯 2K0300 编译部署工具         ║${CLR_RESET}"
#         echo -e "${CLR_BOLD}${CLR_CYAN}  ╚══════════════════════════════════════════╝${CLR_RESET}"
#         echo ""
#         echo -e "  当前配置:"
#         echo -e "    远程设备: ${CLR_CYAN}${REMOTE_USER}@${REMOTE_IP}${CLR_RESET}"
#         echo -e "    上传路径: ${CLR_CYAN}${REMOTE_PATH}${CLR_RESET}"
#         echo -e "    编译线程: ${CLR_CYAN}${MAKE_JOBS}${CLR_RESET}"
#         echo -e "    输出目录: ${CLR_CYAN}${OUT_DIR}${CLR_RESET}"
#         echo ""
#         echo -e "${CLR_BOLD}${CLR_GREEN}  [1] 增量编译 + 上传  ★ 日常最常用${CLR_RESET}"
#         echo -e "  [2] 全量部署（清理→编译→上传）"
#         echo -e "  [3] 仅编译（增量，不上传）"
#         echo -e "  [4] 仅上传（不编译）"
#         echo ""
#         echo -e "  ${CLR_RED}[0] 退出${CLR_RESET}"
#         echo ""
#         echo -n "  请选择操作 [0-4]: "

#         read -r choice
#         echo ""

#         case "$choice" in
#             1) mode_buildup ;;
#             2) mode_all ;;
#             3) mode_build ;;
#             4) mode_upload ;;
#             0) echo -e "${CLR_GREEN}已退出。${CLR_RESET}"; exit 0 ;;
#             *) warn_echo "无效输入，请按回车继续..."; read -r ;;
#         esac

#         echo ""
#         echo -n "  按回车键返回菜单..."
#         read -r
#     done
# }

# # ===================== 入口：参数解析 =====================
# MODE="${1:-menu}"

# case "$MODE" in
#     all)        mode_all ;;
#     buildup|bu) mode_buildup ;;
#     build)      mode_build ;;
#     upload|up)  mode_upload ;;
#     menu|"")    show_menu ;;
#     *)
#         echo "用法: ./build.sh [all|buildup|build|upload]"
#         echo "  无参数      启动交互菜单"
#         echo "  all         全量部署（清理→编译→上传）"
#         echo "  buildup/bu  增量编译+上传"
#         echo "  build       仅编译"
#         echo "  upload/up   仅上传"
#         exit 1
#         ;;
# esac





# #!/bin/bash
# # 脚本功能：支持四种模式
# #   ./build.sh           -> 清理out -> cmake -> make -> 上传（默认，兼容原快捷键）
# #   ./build.sh build     -> 仅 cmake -> make（增量编译，不清理不上传）
# #   ./build.sh buildup   -> 增量编译 -> 上传（不清理，最常用）
# #   ./build.sh upload    -> 仅上传（不清理不编译）
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # ===================== 模式解析 =====================
# MODE="${1:-all}"  # 无参数时默认 all

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "\033[31m[ERROR] $1\033[0m"
#     exit 1
# }

# info_echo() {
#     echo -e "\033[32m[INFO] $1\033[0m"
# }

# # ===================== 编译函数（复用） =====================
# do_cmake_make() {
#     if [ "$MODE" = "build" ] || [ "$MODE" = "buildup" ] || [ "$MODE" = "bu" ]; then
#         if [ ! -d "${OUT_DIR}" ]; then
#             info_echo "创建编译目录: ${OUT_DIR}"
#             mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#         fi
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查 CMakeLists.txt 或编译依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败，编译日志如上！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数（复用） =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传文件/目录：${parent_dir_name}"

#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到待上传的内容: ${parent_dir_name}，请先执行编译！"
#     fi

#     info_echo "开始上传到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 主逻辑 =====================
# case "$MODE" in

#     # ---------- 模式1：只编译（增量，不上传） ----------
#     build)
#         info_echo "【增量编译模式】不清理目录，不执行上传"
#         do_cmake_make
#         info_echo "✅ 增量编译完成，输出目录: ${OUT_DIR}"
#         exit 0
#         ;;

#     # ---------- 模式2：增量编译 + 上传（不清理） ----------
#     buildup|bu)
#         info_echo "【增量编译+上传模式】不清理目录，编译后自动上传"
#         do_cmake_make
#         do_upload
#         info_echo "✅ 增量编译并上传完成！"
#         exit 0
#         ;;

#     # ---------- 模式3：只上传（不清理不编译） ----------
#     upload|up)
#         info_echo "【一键上传模式】仅上传已有编译产物"
#         do_upload
#         info_echo "✅ 上传完成！"
#         exit 0
#         ;;

#     # ---------- 模式4：全流程（默认，兼容原快捷键） ----------
#     all|"")
#         info_echo "【全量部署模式】准备进入目录: ${OUT_DIR}"
#         cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

#         info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
#         find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#             || error_exit "目录清理失败，请检查目录权限！"

#         do_cmake_make
#         do_upload

#         info_echo "✅ 所有操作执行完成：清理目录 → cmake编译 → make编译 → 文件上传 均成功！"
#         exit 0
#         ;;

#     # ---------- 未知参数 ----------
#     *)
#         echo "用法: ./build.sh [all|build|buildup|upload]"
#         echo "  all       (默认) 清理 -> 编译 -> 上传"
#         echo "  build            仅编译（增量，不清理不上传）"
#         echo "  buildup 或 bu    增量编译 -> 上传（不清理，日常最常用）"
#         echo "  upload 或 up     仅上传（不清理不编译）"
#         exit 1
#         ;;
# esac






# #!/bin/bash
# # 脚本功能：支持三种模式
# #   ./build.sh           -> 清理out -> cmake -> make -> 上传（默认，兼容原快捷键）
# #   ./build.sh build     -> 仅 cmake -> make（增量编译，不清理不上传）
# #   ./build.sh upload    -> 仅上传（不清理不编译，直接scp已有产物）
# #   ./build.sh up        -> upload 的简写
# # 注意：增量编译省时间但有隐蔽风险，一旦改了头文件/CMake/静态库，务必用全量模式。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # ===================== 模式解析 =====================
# MODE="${1:-all}"  # 无参数时默认 all

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "\033[31m[ERROR] $1\033[0m"
#     exit 1
# }

# info_echo() {
#     echo -e "\033[32m[INFO] $1\033[0m"
# }

# # ===================== 编译函数（复用） =====================
# do_cmake_make() {
#     # 校验并进入编译目录
#     if [ "$MODE" = "build" ] && [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查 CMakeLists.txt 或编译依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败，编译日志如上！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 上传函数（复用） =====================
# do_upload() {
#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     parent_dir_name=$(basename "$(dirname "$(pwd)")")
#     info_echo "待上传文件/目录：${parent_dir_name}"

#     # 校验产物是否存在
#     if [ ! -e "${parent_dir_name}" ]; then
#         error_exit "找不到待上传的内容: ${parent_dir_name}，请先执行编译！"
#     fi

#     info_echo "开始上传到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#     scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#         || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

#     info_echo "✅ 上传完成"
# }

# # ===================== 主逻辑 =====================
# case "$MODE" in

#     # ---------- 模式1：只编译（增量，不清理不上传） ----------
#     build)
#         info_echo "【增量编译模式】不清理目录，不执行上传"
#         do_cmake_make
#         info_echo "✅ 增量编译完成，输出目录: ${OUT_DIR}"
#         exit 0
#         ;;

#     # ---------- 模式2：只上传（不清理不编译） ----------
#     upload|up)
#         info_echo "【一键上传模式】仅上传已有编译产物"
#         do_upload
#         info_echo "✅ 上传完成！"
#         exit 0
#         ;;

#     # ---------- 模式3：全流程（默认，兼容原快捷键） ----------
#     all|"")
#         # 1. 清理
#         info_echo "【全量部署模式】准备进入目录: ${OUT_DIR}"
#         cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

#         info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
#         find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#             || error_exit "目录清理失败，请检查目录权限！"

#         # 2. 编译
#         do_cmake_make

#         # 3. 上传
#         do_upload

#         info_echo "✅ 所有操作执行完成：清理目录 → cmake编译 → make编译 → 文件上传 均成功！"
#         exit 0
#         ;;

#     # ---------- 未知参数 ----------
#     *)
#         echo "用法: ./build.sh [all|build|upload|up]"
#         echo "  all       (默认) 清理 -> 编译 -> 上传"
#         echo "  build            仅编译（增量，不清理不上传）"
#         echo "  upload 或 up     仅上传（不清理不编译，直接scp已有产物）"
#         exit 1
#         ;;
# esac





# #!/bin/bash
# # 脚本功能：支持两种模式
# #   ./build.sh        -> 清理out -> cmake -> make -> 上传（默认，兼容原快捷键）
# #   ./build.sh build  -> 仅 cmake -> make（增量编译，不清理不上传）
# #增量编译省时间但有隐蔽风险,保持 ./build.sh（全量）不动是对的；
# #日常调试可以用./build.sh build 提速，但一旦改了头文件、CMake 配置、或静态库，务必全量编译。

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区 =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# OUT_DIR="${WORK_DIR}/../out"
# USER_DIR="${WORK_DIR}/../user"
# REMOTE_IP="192.168.1.104"
# REMOTE_USER="root"
# REMOTE_PATH="/home/root/"
# MAKE_JOBS=$(nproc)
# RESERVE_FILE="本文件夹作用.txt"

# # ===================== 模式解析 =====================
# MODE="${1:-all}"  # 无参数时默认 all

# # ===================== 通用函数 =====================
# error_exit() {
#     echo -e "\033[31m[ERROR] $1\033[0m"
#     exit 1
# }

# info_echo() {
#     echo -e "\033[32m[INFO] $1\033[0m"
# }

# # ===================== 编译函数（复用） =====================
# do_cmake_make() {
#     # 校验并进入编译目录
#     if [ "$MODE" = "build" ] && [ ! -d "${OUT_DIR}" ]; then
#         info_echo "创建编译目录: ${OUT_DIR}"
#         mkdir -p "${OUT_DIR}" || error_exit "无法创建 ${OUT_DIR}"
#     fi

#     cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR}"

#     info_echo "执行 cmake: ${USER_DIR}"
#     cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查 CMakeLists.txt 或编译依赖！"

#     info_echo "执行 make -j${MAKE_JOBS} ..."
#     make -j${MAKE_JOBS} || error_exit "make 编译失败，编译日志如上！"

#     info_echo "✅ 编译成功"
# }

# # ===================== 主逻辑 =====================
# case "$MODE" in

#     # ---------- 模式1：只编译（增量，不清理不上传） ----------
#     build)
#         info_echo "【增量编译模式】不清理目录，不执行上传"
#         do_cmake_make
#         info_echo "✅ 增量编译完成，输出目录: ${OUT_DIR}"
#         exit 0
#         ;;

#     # ---------- 模式2：全流程（默认，兼容原快捷键） ----------
#     all|"")
#         # 1. 清理
#         info_echo "【全量部署模式】准备进入目录: ${OUT_DIR}"
#         cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

#         info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
#         find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + \
#             || error_exit "目录清理失败，请检查目录权限！"

#         # 2. 编译
#         do_cmake_make

#         # 3. 上传
#         parent_dir_name=$(basename "$(dirname "$(pwd)")")
#         info_echo "待上传文件/目录：${parent_dir_name}"

#         info_echo "开始上传文件到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
#         scp -O -r "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
#             || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

#         info_echo "✅ 所有操作执行完成：清理目录 → cmake编译 → make编译 → 文件上传 均成功！"
#         exit 0
#         ;;

#     # ---------- 未知参数 ----------
#     *)
#         echo "用法: ./build.sh [all|build]"
#         echo "  all   (默认) 清理 -> 编译 -> 上传"
#         echo "  build        仅编译（增量，不清理不上传）"
#         exit 1
#         ;;
# esac








# #!/bin/bash
# # 脚本功能：清理out目录无关文件 -> cmake编译 -> 编译项目 -> 上传目标文件到远程设备

# export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# # ===================== 核心配置区（可根据需要修改，一目了然） =====================
# WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"  # 脚本所在绝对路径（关键修复）
# OUT_DIR="${WORK_DIR}/../out"                              # out目录绝对路径
# USER_DIR="${WORK_DIR}/../user"                            # user目录绝对路径
# REMOTE_IP="192.168.1.104"                                  # 远程设备IP
# REMOTE_USER="root"                                        # 远程登录用户
# REMOTE_PATH="/home/root/"                                 # 远程上传路径
# MAKE_JOBS=$(nproc)                                        # make编译线程数
# RESERVE_FILE="本文件夹作用.txt"                            # 保留的文件，不删除

# # ===================== 全局通用函数（复用性强，减少冗余代码） =====================
# # 错误退出函数：打印错误信息 + 退出脚本（退出码1）
# error_exit() {
#     echo -e "\033[31m[ERROR] $1\033[0m"
#     exit 1
# }

# # 成功提示函数：打印普通成功信息
# info_echo() {
#     echo -e "\033[32m[INFO] $1\033[0m"
# }

# # ===================== 脚本主逻辑 =====================
# # 1. 进入out目录并校验（修复原脚本cd相对路径的致命bug）
# info_echo "准备进入目录: ${OUT_DIR}"
# cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

# # 2. 清理out目录下所有内容，仅保留指定文件（修复原脚本错误的校验逻辑）
# info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
# find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + || error_exit "目录清理失败，请检查目录权限！"

# # 3. 执行cmake编译，生成Makefile并校验执行结果
# info_echo "执行cmake编译: cmake ${USER_DIR}"
# cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查CMakeLists.txt或编译依赖！"

# # 4. 执行make多线程编译，核心编译步骤
# info_echo "cmake执行成功，开始执行 make -j${MAKE_JOBS} 编译项目..."
# make -j${MAKE_JOBS} || error_exit "make 编译失败，编译日志如上！"

# # 5. 获取上级目录名称（原逻辑保留，优化写法更健壮）
# parent_dir_name=$(basename "$(dirname "$(pwd)")")
# info_echo "待上传文件/目录：${parent_dir_name}"

# # 6. SCP上传文件到远程设备，-O 兼容老版本openssh，增加上传校验
# info_echo "开始上传文件到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
# scp -O "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

# # 7. 全部执行完成
# info_echo "✅ 所有操作执行完成：清理目录 → cmake编译 → make编译 → 文件上传 均成功！"
# exit 0