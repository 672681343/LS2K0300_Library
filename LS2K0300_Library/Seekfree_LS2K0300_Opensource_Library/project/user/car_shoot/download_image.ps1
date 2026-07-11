# ============================================
# download_image.ps1 - 下载原始分类图片（旧格式 + 增补格式分离）
# ============================================

# 宏定义 - 路径配置
$BASE_DIR      = "C:\Users\YY\Desktop\tensorflow\car_shoot\fist_classify"
$REMOTE_USER   = "kaka"
$REMOTE_HOST   = "192.168.229.131"
$REMOTE_BASE   = "/home/kaka/Desktop/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user/image_shoot"

# 下载分类列表
$CATEGORIES = @("materials", "traffic", "weapon")

# ============================================
# 主程序
# ============================================

Write-Host "========================================" -ForegroundColor DarkYellow
Write-Host "      Download Original Images          " -ForegroundColor DarkYellow
Write-Host "========================================" -ForegroundColor DarkYellow

$downloadStats = @()

foreach ($category in $CATEGORIES) {
    Write-Host "`n[$category] Downloading..." -ForegroundColor DarkCyan

    # ========== 旧格式 ==========
    $destDir = Join-Path $BASE_DIR $category
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null

    $remotePath = "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/${category}_*.bmp"
    
    try {
        scp $remotePath $destDir 2>$null
    } catch {
        Write-Warning "SCP command failed for ${category}_*.bmp"
    }

    $files = Get-ChildItem -Path $destDir -File | Where-Object { 
        $_.Name -like "${category}_*.bmp" -and $_.Name -notlike "add_*"
    }
    $oldCount = $files.Count

    # ========== 增补格式（add_前缀）==========
    $destDirAdd = Join-Path $BASE_DIR "add_$category"
    New-Item -ItemType Directory -Path $destDirAdd -Force | Out-Null

    $remotePathAdd = "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/add_${category}_*.bmp"
    
    try {
        scp $remotePathAdd $destDirAdd 2>$null
    } catch {
        Write-Warning "SCP command failed for add_${category}_*.bmp"
    }

    $filesAdd = Get-ChildItem -Path $destDirAdd -File | Where-Object { 
        $_.Name -like "add_${category}_*.bmp"
    }
    $addCount = $filesAdd.Count

    # 输出结果
    if ($oldCount -gt 0) {
        Write-Host "  -> Downloaded $oldCount files" -ForegroundColor DarkGreen
    } else {
        Write-Warning "  $category download failed - no files found"
    }

    if ($addCount -gt 0) {
        Write-Host "  -> Add Downloaded $addCount files" -ForegroundColor Cyan
    }

    $downloadStats += [PSCustomObject]@{
        Category = $category
        Count    = $oldCount
        AddCount = $addCount
    }
}

# ============================================
# 统计结果
# ============================================

Write-Host "`n========================================" -ForegroundColor DarkYellow
Write-Host "           Download Summary               " -ForegroundColor DarkYellow
Write-Host "========================================" -ForegroundColor DarkYellow

$totalFiles = 0
$totalAdd = 0

foreach ($stat in $downloadStats) {
    Write-Host "  $($stat.Category) : $($stat.Count) files" -ForegroundColor DarkGray
    if ($stat.AddCount -gt 0) {
        Write-Host "  add_$($stat.Category) : $($stat.AddCount) files" -ForegroundColor Cyan
    }
    $totalFiles += $stat.Count
    $totalAdd += $stat.AddCount
}

Write-Host "`n  Total downloaded: $totalFiles files" -ForegroundColor DarkGray
if ($totalAdd -gt 0) {
    Write-Host "  Total add downloaded: $totalAdd files" -ForegroundColor Cyan
}

Write-Host "`n========================================" -ForegroundColor DarkYellow
Write-Host "All downloads finished!" -ForegroundColor DarkYellow
Write-Host "Saved to: $BASE_DIR" -ForegroundColor DarkCyan