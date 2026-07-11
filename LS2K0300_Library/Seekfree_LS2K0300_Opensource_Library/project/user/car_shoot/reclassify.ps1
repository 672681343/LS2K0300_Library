# ============================================
# reclassify.ps1 - 本地图片重新分类(目前用不到)
# ============================================

# 宏定义 - 路径配置
$BASE_DIR       = "C:\Users\YY\Desktop\tensorflow\car_shoot\fist_classify"
$RECLASSIFY_DIR = Join-Path $BASE_DIR "reclassify"

# 源目录
$SRC_MATERIALS = Join-Path $BASE_DIR "materials"
$SRC_TRAFFIC   = Join-Path $BASE_DIR "traffic"
$SRC_WEAPON    = Join-Path $BASE_DIR "weapon"

# 分类映射定义: @(源目录, 匹配后缀, 目标大类, 目标子文件夹)
$CLASS_MAP = @(
    @($SRC_MATERIALS, "_1.jpg", "materials", "scope_1"),
    @($SRC_MATERIALS, "_2.jpg", "materials", "medkit_2"),
    @($SRC_TRAFFIC,   "_1.jpg", "traffic",   "tank_1"),
    @($SRC_TRAFFIC,   "_2.jpg", "traffic",   "ambulance_2"),
    @($SRC_WEAPON,    "_1.jpg", "weapon",    "gun_1"),
    @($SRC_WEAPON,    "_2.jpg", "weapon",    "explosive_2")
)

function Reclassify-Images {
    param(
        [string]$SrcDir,
        [string]$Suffix,
        [string]$DstCategory,
        [string]$DstSubDir
    )

    if (-not (Test-Path $SrcDir)) {
        Write-Warning "Source directory not found: $SrcDir"
        return 0
    }

    $dstDir = Join-Path (Join-Path $RECLASSIFY_DIR $DstCategory) $DstSubDir
    New-Item -ItemType Directory -Path $dstDir -Force | Out-Null

    Write-Host "`n[$DstCategory\$DstSubDir] suffix=$Suffix" -ForegroundColor DarkCyan

    $pattern = "*$Suffix"
    $files = Get-ChildItem -Path $SrcDir -Filter $pattern -ErrorAction SilentlyContinue

    if ($files.Count -eq 0) {
        Write-Warning "No files matching '$pattern' in $SrcDir"
        return 0
    }

    $copiedCount = 0
    foreach ($file in $files) {
        $dstPath = Join-Path $dstDir $file.Name
        Copy-Item -Path $file.FullName -Destination $dstPath -Force
        $copiedCount++
    }

    Write-Host "  -> Copied $copiedCount files" -ForegroundColor DarkGreen
    return $copiedCount
}

# ============================================
# 主程序
# ============================================

Write-Host "========================================" -ForegroundColor DarkYellow
Write-Host "       Local Image Reclassification     " -ForegroundColor DarkYellow
Write-Host "========================================" -ForegroundColor DarkYellow

$copyStats = @()

foreach ($map in $CLASS_MAP) {
    $count = Reclassify-Images -SrcDir $map[0] -Suffix $map[1] -DstCategory $map[2] -DstSubDir $map[3]
    $copyStats += [PSCustomObject]@{
        Category    = $map[2]
        SubDir      = $map[3]
        CopiedCount = $count
    }
}

# ============================================
# 详细统计结果
# ============================================

Write-Host "`n========================================" -ForegroundColor DarkYellow
Write-Host "           Detailed Statistics            " -ForegroundColor DarkYellow
Write-Host "========================================" -ForegroundColor DarkYellow

# 1. 源目录总文件数
Write-Host "`n[Source Directories - Total Files]" -ForegroundColor DarkCyan

$srcDirs = @(
    @("materials", $SRC_MATERIALS),
    @("traffic",   $SRC_TRAFFIC),
    @("weapon",    $SRC_WEAPON)
)

foreach ($item in $srcDirs) {
    $name = $item[0]
    $path = $item[1]
    if (Test-Path $path) {
        $total = (Get-ChildItem $path -Filter "*.jpg" -ErrorAction SilentlyContinue).Count
        Write-Host "  $name : $total files" -ForegroundColor DarkGray
    } else {
        Write-Host "  $name : directory not found" -ForegroundColor DarkRed
    }
}

# 2. 重新分类后各子目录文件数
Write-Host "`n[Reclassified Directories]" -ForegroundColor DarkCyan

foreach ($stat in $copyStats) {
    $path = Join-Path (Join-Path $RECLASSIFY_DIR $stat.Category) $stat.SubDir
    $count = (Get-ChildItem $path -ErrorAction SilentlyContinue).Count
    Write-Host "  $($stat.Category)\$($stat.SubDir) : $count files (copied: $($stat.CopiedCount))" -ForegroundColor DarkGray
}

# 3. 汇总
Write-Host "`n[Summary]" -ForegroundColor DarkCyan

$totalSrc = 0
foreach ($item in $srcDirs) {
    $path = $item[1]
    if (Test-Path $path) {
        $totalSrc += (Get-ChildItem $path -Filter "*.jpg" -ErrorAction SilentlyContinue).Count
    }
}

$totalDst = 0
foreach ($stat in $copyStats) {
    $path = Join-Path (Join-Path $RECLASSIFY_DIR $stat.Category) $stat.SubDir
    $totalDst += (Get-ChildItem $path -ErrorAction SilentlyContinue).Count
}

Write-Host "  Total source files       : $totalSrc" -ForegroundColor DarkGray
Write-Host "  Total reclassified files : $totalDst" -ForegroundColor DarkGray

if ($totalSrc -eq $totalDst) {
    Write-Host "  Status: All files reclassified successfully!" -ForegroundColor DarkGreen
} else {
    Write-Host "  Status: Some files may not match the pattern! (diff: $($totalDst - $totalSrc))" -ForegroundColor DarkRed
}

Write-Host "`n========================================" -ForegroundColor DarkYellow
Write-Host "Reclassification finished!" -ForegroundColor DarkYellow
Write-Host "Output: $RECLASSIFY_DIR" -ForegroundColor DarkCyan