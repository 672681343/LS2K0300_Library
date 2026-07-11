# ============================================
# reclassify.ps1 - 本地图片重新分类（支持两种模式）
# ============================================

# 修复中文乱码
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$PSDefaultParameterValues['Out-File:Encoding'] = 'utf8'

# 宏定义 - 路径配置
$SRC_DIR        = "C:\Users\YY\Desktop\tensorflow\car_shoot\fist_classify"
$RECLASSIFY_DIR = "C:\Users\YY\Desktop\tensorflow\car_shoot\reclassify"
$CLASSIFY_V3    = "C:\Users\YY\Desktop\tensorflow\car_shoot\classify_v3"

# 源目录（旧格式 + 增补格式）
$SRC_MATERIALS    = Join-Path $SRC_DIR "materials"
$SRC_TRAFFIC      = Join-Path $SRC_DIR "traffic"
$SRC_WEAPON       = Join-Path $SRC_DIR "weapon"
$SRC_ADD_MATERIALS = Join-Path $SRC_DIR "add_materials"
$SRC_ADD_TRAFFIC   = Join-Path $SRC_DIR "add_traffic"
$SRC_ADD_WEAPON    = Join-Path $SRC_DIR "add_weapon"

# ============================================
# 分类模式定义
# ============================================

# 模式A: 更细分类 - 输出到 classify_v3
# 目录结构: 大类/子类_大类编号_背景编号
$MODE_A_MAP = @(
    # materials 旧格式
    @($SRC_MATERIALS, "materials_p1_1_*.bmp", "materials", "telescope_1_1"),
    @($SRC_MATERIALS, "materials_p2_1_*.bmp", "materials", "medkit_2_1"),
    @($SRC_MATERIALS, "materials_p2_2_*.bmp", "materials", "medkit_2_2"),
    # materials 增补格式
    @($SRC_ADD_MATERIALS, "add_materials_p1_1_*.bmp", "materials", "telescope_1_1"),
    @($SRC_ADD_MATERIALS, "add_materials_p2_1_*.bmp", "materials", "medkit_2_1"),
    @($SRC_ADD_MATERIALS, "add_materials_p2_2_*.bmp", "materials", "medkit_2_2"),
    # traffic 旧格式（armored_vehicle 改为 tank）
    @($SRC_TRAFFIC, "traffic_p1_1_*.bmp", "traffic", "tank_1_1"),
    @($SRC_TRAFFIC, "traffic_p1_2_*.bmp", "traffic", "tank_1_2"),
    @($SRC_TRAFFIC, "traffic_p2_1_*.bmp", "traffic", "ambulance_2_1"),
    @($SRC_TRAFFIC, "traffic_p2_2_*.bmp", "traffic", "ambulance_2_2"),
    # traffic 增补格式
    @($SRC_ADD_TRAFFIC, "add_traffic_p1_1_*.bmp", "traffic", "tank_1_1"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p1_2_*.bmp", "traffic", "tank_1_2"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p2_1_*.bmp", "traffic", "ambulance_2_1"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p2_2_*.bmp", "traffic", "ambulance_2_2"),
    # weapon 旧格式
    @($SRC_WEAPON, "weapon_p1_1_*.bmp", "weapon", "gun_1_1"),
    @($SRC_WEAPON, "weapon_p1_2_*.bmp", "weapon", "gun_1_2"),
    @($SRC_WEAPON, "weapon_p2_1_*.bmp", "weapon", "explosive_2_1"),
    @($SRC_WEAPON, "weapon_p2_2_*.bmp", "weapon", "explosive_2_2"),
    @($SRC_WEAPON, "weapon_p2_3_*.bmp", "weapon", "explosive_2_3"),
    # weapon 增补格式
    @($SRC_ADD_WEAPON, "add_weapon_p1_1_*.bmp", "weapon", "gun_1_1"),
    @($SRC_ADD_WEAPON, "add_weapon_p1_2_*.bmp", "weapon", "gun_1_2"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_1_*.bmp", "weapon", "explosive_2_1"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_2_*.bmp", "weapon", "explosive_2_2"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_3_*.bmp", "weapon", "explosive_2_3")
)

# 模式B: 六分类 - 输出到 reclassify
# 目录结构: 大类/子类_大类编号（只有一个尾缀数字）
$MODE_B_MAP = @(
    # materials 旧格式
    @($SRC_MATERIALS, "materials_p1_1_*.bmp", "materials", "telescope_1"),
    @($SRC_MATERIALS, "materials_p2_1_*.bmp", "materials", "medkit_2"),
    @($SRC_MATERIALS, "materials_p2_2_*.bmp", "materials", "medkit_2"),
    # materials 增补格式
    @($SRC_ADD_MATERIALS, "add_materials_p1_1_*.bmp", "materials", "telescope_1"),
    @($SRC_ADD_MATERIALS, "add_materials_p2_1_*.bmp", "materials", "medkit_2"),
    @($SRC_ADD_MATERIALS, "add_materials_p2_2_*.bmp", "materials", "medkit_2"),
    # traffic 旧格式（armored_vehicle 改为 tank）
    @($SRC_TRAFFIC, "traffic_p1_1_*.bmp", "traffic", "tank_1"),
    @($SRC_TRAFFIC, "traffic_p1_2_*.bmp", "traffic", "tank_1"),
    @($SRC_TRAFFIC, "traffic_p2_1_*.bmp", "traffic", "ambulance_2"),
    @($SRC_TRAFFIC, "traffic_p2_2_*.bmp", "traffic", "ambulance_2"),
    # traffic 增补格式
    @($SRC_ADD_TRAFFIC, "add_traffic_p1_1_*.bmp", "traffic", "tank_1"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p1_2_*.bmp", "traffic", "tank_1"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p2_1_*.bmp", "traffic", "ambulance_2"),
    @($SRC_ADD_TRAFFIC, "add_traffic_p2_2_*.bmp", "traffic", "ambulance_2"),
    # weapon 旧格式
    @($SRC_WEAPON, "weapon_p1_1_*.bmp", "weapon", "gun_1"),
    @($SRC_WEAPON, "weapon_p1_2_*.bmp", "weapon", "gun_1"),
    @($SRC_WEAPON, "weapon_p2_1_*.bmp", "weapon", "explosive_2"),
    @($SRC_WEAPON, "weapon_p2_2_*.bmp", "weapon", "explosive_2"),
    @($SRC_WEAPON, "weapon_p2_3_*.bmp", "weapon", "explosive_2"),
    # weapon 增补格式
    @($SRC_ADD_WEAPON, "add_weapon_p1_1_*.bmp", "weapon", "gun_1"),
    @($SRC_ADD_WEAPON, "add_weapon_p1_2_*.bmp", "weapon", "gun_1"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_1_*.bmp", "weapon", "explosive_2"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_2_*.bmp", "weapon", "explosive_2"),
    @($SRC_ADD_WEAPON, "add_weapon_p2_3_*.bmp", "weapon", "explosive_2")
)

# ============================================
# 功能函数（保持不变）
# ============================================

function Reclassify-ModeA {
    param([array]$Map)
    $copyStats = @()
    foreach ($item in $Map) {
        $srcDir      = $item[0]
        $pattern     = $item[1]
        $dstCategory = $item[2]
        $dstSubDir   = $item[3]
        if (-not (Test-Path $srcDir)) {
            Write-Warning "Source directory not found: $srcDir"
            continue
        }
        $dstDir = Join-Path (Join-Path $CLASSIFY_V3 $dstCategory) $dstSubDir
        New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
        Write-Host "`n[$dstCategory\$dstSubDir] pattern=$pattern" -ForegroundColor DarkBlue
        $files = Get-ChildItem -Path $srcDir -Filter $pattern -ErrorAction SilentlyContinue
        if ($files.Count -eq 0) {
            Write-Warning "No files matching '$pattern' in $srcDir"
        }
        $copiedCount = 0
        foreach ($file in $files) {
            $dstPath = Join-Path $dstDir $file.Name
            Copy-Item -Path $file.FullName -Destination $dstPath -Force
            $copiedCount++
        }
        Write-Host "  -> Copied $copiedCount files" -ForegroundColor DarkGreen
        $copyStats += [PSCustomObject]@{
            Category    = $dstCategory
            SubDir      = $dstSubDir
            CopiedCount = $copiedCount
        }
    }
    return $copyStats
}

function Reclassify-ModeB {
    param([array]$Map)
    $copyStats = @()
    foreach ($item in $Map) {
        $srcDir      = $item[0]
        $pattern     = $item[1]
        $dstCategory = $item[2]
        $dstSubDir   = $item[3]
        if (-not (Test-Path $srcDir)) {
            Write-Warning "Source directory not found: $srcDir"
            continue
        }
        $dstDir = Join-Path (Join-Path $RECLASSIFY_DIR $dstCategory) $dstSubDir
        New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
        Write-Host "`n[$dstCategory\$dstSubDir] pattern=$pattern" -ForegroundColor DarkBlue
        $files = Get-ChildItem -Path $srcDir -Filter $pattern -ErrorAction SilentlyContinue
        if ($files.Count -eq 0) {
            Write-Warning "No files matching '$pattern' in $srcDir"
        }
        $copiedCount = 0
        foreach ($file in $files) {
            $dstPath = Join-Path $dstDir $file.Name
            Copy-Item -Path $file.FullName -Destination $dstPath -Force
            $copiedCount++
        }
        Write-Host "  -> Copied $copiedCount files" -ForegroundColor DarkGreen
        $copyStats += [PSCustomObject]@{
            Category    = $dstCategory
            SubDir      = $dstSubDir
            CopiedCount = $copiedCount
        }
    }
    return $copyStats
}

function Show-Statistics-ModeA {
    param([array]$Stats)
    Write-Host "`n========================================" -ForegroundColor Black
    Write-Host "     Mode A - Fine Classification       " -ForegroundColor Black
    Write-Host "     Output: $CLASSIFY_V3               " -ForegroundColor DarkGray
    Write-Host "========================================" -ForegroundColor Black
    $srcDirs = @(
        @("materials", $SRC_MATERIALS),
        @("traffic",   $SRC_TRAFFIC),
        @("weapon",    $SRC_WEAPON),
        @("add_materials", $SRC_ADD_MATERIALS),
        @("add_traffic",   $SRC_ADD_TRAFFIC),
        @("add_weapon",    $SRC_ADD_WEAPON)
    )
    $totalSrc = 0
    foreach ($item in $srcDirs) {
        $name = $item[0]
        $path = $item[1]
        if (Test-Path $path) {
            $total = (Get-ChildItem $path -Filter "*.bmp" -ErrorAction SilentlyContinue).Count
            Write-Host "  $name : $total files" -ForegroundColor DarkGray
            $totalSrc += $total
        } else {
            Write-Host "  $name : directory not found" -ForegroundColor DarkRed
        }
    }
    Write-Host "`n[Reclassified Directories]" -ForegroundColor Black
    $totalDst = 0
    foreach ($stat in $Stats) {
        $path = Join-Path (Join-Path $CLASSIFY_V3 $stat.Category) $stat.SubDir
        $count = (Get-ChildItem $path -ErrorAction SilentlyContinue).Count
        $totalDst += $count
        Write-Host "  $($stat.Category)\$($stat.SubDir) : $count files (copied: $($stat.CopiedCount))" -ForegroundColor DarkGray
    }
    Write-Host "`n[Summary]" -ForegroundColor Black
    Write-Host "  Total source files       : $totalSrc" -ForegroundColor DarkGray
    Write-Host "  Total reclassified files : $totalDst" -ForegroundColor DarkGray
    if ($totalSrc -eq $totalDst) {
        Write-Host "  Status: All files reclassified successfully!" -ForegroundColor DarkGreen
    } else {
        Write-Host "  Status: Some files may not match the pattern! (diff: $($totalDst - $totalSrc))" -ForegroundColor DarkRed
    }
}

function Show-Statistics-ModeB {
    param([array]$Stats)
    Write-Host "`n========================================" -ForegroundColor Black
    Write-Host "     Mode B - Six-Class Classification    " -ForegroundColor Black
    Write-Host "     Output: $RECLASSIFY_DIR              " -ForegroundColor DarkGray
    Write-Host "========================================" -ForegroundColor Black
    $srcDirs = @(
        @("materials", $SRC_MATERIALS),
        @("traffic",   $SRC_TRAFFIC),
        @("weapon",    $SRC_WEAPON),
        @("add_materials", $SRC_ADD_MATERIALS),
        @("add_traffic",   $SRC_ADD_TRAFFIC),
        @("add_weapon",    $SRC_ADD_WEAPON)
    )
    $totalSrc = 0
    foreach ($item in $srcDirs) {
        $name = $item[0]
        $path = $item[1]
        if (Test-Path $path) {
            $total = (Get-ChildItem $path -Filter "*.bmp" -ErrorAction SilentlyContinue).Count
            Write-Host "  $name : $total files" -ForegroundColor DarkGray
            $totalSrc += $total
        } else {
            Write-Host "  $name : directory not found" -ForegroundColor DarkRed
        }
    }
    Write-Host "`n[Reclassified Directories]" -ForegroundColor Black
    $totalDst = 0
    foreach ($stat in $Stats) {
        $path = Join-Path (Join-Path $RECLASSIFY_DIR $stat.Category) $stat.SubDir
        $count = (Get-ChildItem $path -ErrorAction SilentlyContinue).Count
        $totalDst += $count
        Write-Host "  $($stat.Category)\$($stat.SubDir) : $count files (copied: $($stat.CopiedCount))" -ForegroundColor DarkGray
    }
    Write-Host "`n[Summary]" -ForegroundColor Black
    Write-Host "  Total source files       : $totalSrc" -ForegroundColor DarkGray
    Write-Host "  Total reclassified files : $totalDst" -ForegroundColor DarkGray
    if ($totalSrc -eq $totalDst) {
        Write-Host "  Status: All files reclassified successfully!" -ForegroundColor DarkGreen
    } else {
        Write-Host "  Status: Some files may not match the pattern! (diff: $($totalDst - $totalSrc))" -ForegroundColor DarkRed
    }
}

# ============================================
# 主程序 - 模式选择
# ============================================

Write-Host "========================================" -ForegroundColor Black
Write-Host "       Local Image Reclassification     " -ForegroundColor Black
Write-Host "========================================" -ForegroundColor Black
Write-Host ""
Write-Host "Source: $SRC_DIR" -ForegroundColor DarkBlue
Write-Host ""
Write-Host "Select classification mode:" -ForegroundColor Black
Write-Host "  [1] Mode A - Fine classification (12 classes)" -ForegroundColor DarkBlue
Write-Host "      Output: $CLASSIFY_V3" -ForegroundColor DarkGray
Write-Host "      Structure: materials/medkit_2_1 (two suffix digits)" -ForegroundColor DarkGray
Write-Host "  [2] Mode B - Six-class classification" -ForegroundColor DarkBlue
Write-Host "      Output: $RECLASSIFY_DIR" -ForegroundColor DarkGray
Write-Host "      Structure: materials/medkit_2 (one suffix digit)" -ForegroundColor DarkGray
Write-Host ""

$mode = Read-Host "Enter mode number (1 or 2)"

switch ($mode) {
    "1" {
        Write-Host "`nRunning Mode A: Fine classification..." -ForegroundColor Black
        $stats = Reclassify-ModeA -Map $MODE_A_MAP
        Show-Statistics-ModeA -Stats $stats
    }
    "2" {
        Write-Host "`nRunning Mode B: Six-class classification..." -ForegroundColor Black
        $stats = Reclassify-ModeB -Map $MODE_B_MAP
        Show-Statistics-ModeB -Stats $stats
    }
    default {
        Write-Host "Invalid mode selected. Please run again and enter 1 or 2." -ForegroundColor DarkRed
        exit 1
    }
}

Write-Host "`n========================================" -ForegroundColor Black
Write-Host "Reclassification finished!" -ForegroundColor Black