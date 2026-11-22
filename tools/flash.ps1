<#
.SYNOPSIS
    自动搜索 ESP32-S3 串口并烧录键盘副屏固件（Windows / PowerShell）。

.DESCRIPTION
    本脚本会：
      1. 自动找出键盘副屏所在的串口（排除蓝牙串口，优先匹配 Espressif VID_303A）；
      2. 检查 ESP-IDF 环境是否已导出（找不到 idf.py 会给出提示）；
      3. 可选先编译，再烧录，最后看情况打开串口监视。

.PARAMETER Port
    手动指定串口，例如 COM7。不填则自动搜索。

.PARAMETER Baud
    烧录波特率，默认 460800。若烧录不稳定可降到 115200。

.PARAMETER NoBuild
    跳过编译，直接烧录现有 build 产物。

.PARAMETER Monitor
    烧录完成后打开 idf.py monitor（Ctrl+] 退出）。

.EXAMPLE
    .\tools\flash.ps1

.EXAMPLE
    .\tools\flash.ps1 -Port COM7 -Monitor

.NOTES
    首次使用前请先安装 ESP-IDF 5.1.4 并在当前 PowerShell 里执行过 export.ps1，
    详见 docs/FLASHING.md。
#>
[CmdletBinding()]
param(
    [string]$Port = "",
    [int]$Baud = 460800,
    [switch]$NoBuild,
    [switch]$Monitor
)

# idf.py 在 export.ps1 里是个包装函数，它会往 stderr 打 WARNING（比如没装 Git）。
# Stop 模式下这些 stderr 会被当成异常直接中断脚本，所以这里用 Continue，
# 真正的失败靠 $LASTEXITCODE 判断。
$ErrorActionPreference = 'Continue'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

function Write-Step($text) { Write-Host ""; Write-Host "==> $text" -ForegroundColor Cyan }
function Write-Ok($text)   { Write-Host "    $text" -ForegroundColor Green }
function Write-Warn2($text){ Write-Host "    $text" -ForegroundColor Yellow }

# ---------------------------------------------------------------- 1. 环境检查
Write-Step "检查 ESP-IDF 环境"

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    Write-Host "    找不到 idf.py，说明还没导出 ESP-IDF 环境。" -ForegroundColor Red
    Write-Host ""
    Write-Host "    请先在本 PowerShell 窗口执行（把路径换成你自己的）：" -ForegroundColor Yellow
    Write-Host '      $env:IDF_PATH =  "D:\esp\esp-idf-v5.1.4"' -ForegroundColor Gray
    Write-Host '      . "$env:IDF_PATH\export.ps1"' -ForegroundColor Gray
    Write-Host ""
    Write-Host "    若 export.ps1 报 python_env 不存在，再补一行指定 Python 环境：" -ForegroundColor Yellow
    Write-Host '      $env:IDF_PYTHON_ENV_PATH = "$env:IDF_TOOLS_PATH\python_env\idf5.1_py3.11_env"' -ForegroundColor Gray
    Write-Host ""
    Write-Host "    安装与排错细节见 docs/FLASHING.md" -ForegroundColor Yellow
    exit 1
}
Write-Ok "idf.py 可用"

# ---------------------------------------------------------------- 2. 找串口
Write-Step "搜索设备串口"

function Get-KeyboardPorts {
    $found = @()
    try {
        $devices = Get-CimInstance Win32_PnPEntity -ErrorAction Stop |
                   Where-Object { $_.Name -match '\(COM(\d+)\)' }
    } catch {
        return $found
    }

    foreach ($d in $devices) {
        $com = [regex]::Match($d.Name, '\((COM\d+)\)').Groups[1].Value
        if (-not $com) { continue }

        $isBluetooth = ($d.Name -match '蓝牙|Bluetooth') -or ($d.PNPDeviceID -match 'BTHENUM')
        $isEspressif = ($d.PNPDeviceID -match 'VID_303A')
        $isUsbSerial = ($d.Name -match 'USB.*串行|USB.*Serial|CH343|CH340|CP210|USB JTAG')

        if ($isBluetooth) { continue }
        if (-not ($isEspressif -or $isUsbSerial)) { continue }

        $found += [pscustomobject]@{
            Port      = $com
            Name      = $d.Name
            Espressif = $isEspressif
            Present   = ($d.Status -eq 'OK')
        }
    }
    return $found
}

$candidates = Get-KeyboardPorts

if ($Port) {
    Write-Ok "使用手动指定的串口 $Port"
} elseif ($candidates.Count -eq 0) {
    Write-Host "    没有找到任何 USB 串口设备。" -ForegroundColor Red
    Write-Host ""
    Write-Host "    请确认：" -ForegroundColor Yellow
    Write-Host "      1. 键盘用 USB-C 数据线（不是纯充电线）接到电脑" -ForegroundColor Gray
    Write-Host "      2. 拔插一次，让设备重新枚举" -ForegroundColor Gray
    Write-Host "      3. 设备管理器里能看到「USB 串行设备 (COMx)」" -ForegroundColor Gray
    Write-Host "      4. 没装驱动时先装 CH343 / CP210x 驱动" -ForegroundColor Gray
    exit 1
} else {
    $preferred = $candidates | Where-Object { $_.Espressif -and $_.Present } | Select-Object -First 1
    if (-not $preferred) { $preferred = $candidates | Where-Object { $_.Present } | Select-Object -First 1 }
    if (-not $preferred) { $preferred = $candidates | Select-Object -First 1 }
    $Port = $preferred.Port
    Write-Ok "找到 $Port  —  $($preferred.Name)"
    foreach ($c in $candidates) {
        if ($c.Port -ne $Port) { Write-Host "    （忽略 $($c.Port)：$($c.Name)）" -ForegroundColor DarkGray }
    }
}

# ---------------------------------------------------------------- 3. 编译
if (-not $NoBuild) {
    Write-Step "编译固件（首次约 5-10 分钟）"
    idf.py build
    if ($LASTEXITCODE -ne 0) {
        Write-Host "    编译失败，请把上面的错误发到 issue。" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Ok "编译完成"
} else {
    Write-Warn2 "跳过编译（-NoBuild）"
}

# ---------------------------------------------------------------- 4. 烧录
Write-Step "烧录到 $Port（$Baud baud）"
idf.py -p $Port -b $Baud flash
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "    烧录失败，常见原因：" -ForegroundColor Red
    Write-Host "      - 串口被别的软件占用（关掉串口助手 / 另一个 monitor）" -ForegroundColor Gray
    Write-Host "      - 端口号变了，重新跑一次本脚本" -ForegroundColor Gray
    Write-Host "      - 换成低速重试：.\tools\flash.ps1 -Baud 115200" -ForegroundColor Gray
    exit $LASTEXITCODE
}
Write-Ok "烧录完成，设备已自动重启"

# ---------------------------------------------------------------- 5. 监视
if ($Monitor) {
    Write-Step "打开串口监视（Ctrl+] 退出）"
    idf.py -p $Port monitor
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "全部完成。设备已重启，按键 Fn+~ 可在各页面之间切换。" -ForegroundColor Green
Write-Host "需要看日志时运行：idf.py -p $Port monitor" -ForegroundColor DarkGray
