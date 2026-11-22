# 不依赖 ESP-IDF 的烧录脚本（Windows PowerShell）
#
# 用法（在本仓库根目录或任意位置）：
#     .\tools\flash-noidf.ps1                  # 自动找串口并烧录
#     .\tools\flash-noidf.ps1 -Port COM6       # 指定串口
#     .\tools\flash-noidf.ps1 -Erase           # 先整片擦除再烧（首次安装推荐）
#     .\tools\flash-noidf.ps1 -Firmware .\docs\flasher\firmware\ArknightsKeyboardScreen-full.bin
#
# 只需要系统里有 Python 3（脚本会自动装 esptool）。

param(
    [string]$Port = "",
    [int]$Baud = 460800,
    [string]$Firmware = "",
    [switch]$Erase
)

# idf.py 之类的包装函数会把 WARNING 写到 stderr，Stop 模式下会被当异常
$ErrorActionPreference = 'Continue'
$repoRoot = Split-Path -Parent $PSScriptRoot

function Fail([string]$msg) {
    Write-Host ""
    Write-Host "  [错误] $msg" -ForegroundColor Red
    exit 1
}

function Find-Python {
    $candidates = @(
        @{ exe = "python"; args = @() },
        @{ exe = "py"; args = @("-3") },
        @{ exe = "python3"; args = @() }
    )
    foreach ($c in $candidates) {
        $cmd = Get-Command $c.exe -ErrorAction SilentlyContinue
        if (-not $cmd) { continue }
        $ver = & $c.exe @($c.args + @("-c", "import sys;print(sys.version_info[0])")) 2>$null
        if ($ver -match '^\s*3\s*$') {
            return @{ exe = $c.exe; args = $c.args }
        }
    }
    return $null
}

function Get-PortLabel([string]$p) {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object { $_.FriendlyName -match [regex]::Escape("($p)") } | Select-Object -First 1
    if ($dev) { return ($dev.FriendlyName -replace '\s*\([^)]*\)\s*$', '') }
    return '未知设备'
}

function Show-PortTable([string]$chosen) {
    $list = @([System.IO.Ports.SerialPort]::GetPortNames())
    if ($list.Count -le 1) { return }
    Write-Host "    系统里的串口：" -ForegroundColor DarkGray
    foreach ($p in $list) {
        $label = Get-PortLabel $p
        $mark = ''
        if ($p -eq $chosen) { $mark = '   <= 将烧录到这个' }
        Write-Host "      $p  $label$mark" -ForegroundColor DarkGray
    }
}

function Find-KeyboardPort {
    # 依次尝试：CIM 串口表 -> PnP 设备名里的 (COMx) -> 兜底列出全部端口
    $ports = @()

    $cim = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue
    foreach ($p in $cim) {
        if ($p.PNPDeviceID -like '*VID_303A*') { $ports += $p.DeviceID; continue }
        if ($p.Name -match '蓝牙|Bluetooth|BTHENUM') { continue }
        if ($p.Name -match 'USB|串行') { $ports += $p.DeviceID }
    }

    if (-not $ports) {
        $pnp = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
            Where-Object { $_.InstanceId -like '*VID_303A*' -and $_.FriendlyName -match '(COM\d+)' }
        foreach ($d in $pnp) {
            if ($d.FriendlyName -match '(COM\d+)') { $ports += $Matches[1] }
        }
    }

    $ports = @($ports | Select-Object -Unique)
    if ($ports.Count -gt 0) { return $ports[0] }

    # 兜底：排除蓝牙口后取剩下的第一个
    $all = @([System.IO.Ports.SerialPort]::GetPortNames() | Where-Object { $_ -ne 'COM5' })
    if ($all.Count -gt 0) { return $all[0] }
    return $null
}

Write-Host ""
Write-Host "==> 检查 Python" -ForegroundColor Cyan
$py = Find-Python
if (-not $py) {
    Fail "没找到 Python 3。请先安装 Python（https://www.python.org/downloads/），或者直接用网页版刷机：docs/flasher/"
}
Write-Host "    $($py.exe)"

Write-Host ""
Write-Host "==> 检查 esptool" -ForegroundColor Cyan
$esptoolOk = $false
& $py.exe @($py.args + @("-m", "esptool", "version")) *> $null
if ($LASTEXITCODE -eq 0) {
    $esptoolOk = $true
    Write-Host "    已安装"
} else {
    Write-Host "    未安装，正在安装 esptool ..." -ForegroundColor Yellow
    $mirrors = @(
        'https://mirrors.aliyun.com/pypi/simple/',
        'https://pypi.tuna.tsinghua.edu.cn/simple',
        'https://pypi.org/simple/'
    )
    foreach ($m in $mirrors) {
        Write-Host "      源：$m" -ForegroundColor DarkGray
        & $py.exe @($py.args + @("-m", "pip", "install", "--upgrade", "--disable-pip-version-check",
            "--timeout", "20", "--retries", "1", "-i", $m, "esptool"))
        & $py.exe @($py.args + @("-m", "esptool", "version")) *> $null
        if ($LASTEXITCODE -eq 0) { $esptoolOk = $true; break }

        # 有的 Python 连 pip 都没有，先 bootstrap 再试同一个源
        & $py.exe @($py.args + @("-m", "ensurepip", "--upgrade")) *> $null
        & $py.exe @($py.args + @("-m", "pip", "install", "--upgrade", "--disable-pip-version-check",
            "--timeout", "20", "--retries", "1", "-i", $m, "esptool")) *> $null
        & $py.exe @($py.args + @("-m", "esptool", "version")) *> $null
        if ($LASTEXITCODE -eq 0) { $esptoolOk = $true; break }
    }
    if (-not $esptoolOk) {
        Fail "esptool 安装失败。可以手动执行：pip install esptool -i https://mirrors.aliyun.com/pypi/simple/`n      或用网页版刷机（完全不需要 Python）：docs/flasher/"
    }
    Write-Host "    安装完成"
}

$flashPairs = @()
if (-not $Firmware) {
    $Firmware = Join-Path $repoRoot "docs\flasher\firmware"
}
if (-not (Test-Path $Firmware)) {
    Fail "找不到固件：$Firmware`n      请确认在完整仓库里执行，或用 -Firmware 指定。"
}
$Firmware = (Resolve-Path $Firmware).Path

if (Test-Path $Firmware -PathType Container) {
    # 四个镜像分别写，不做合并 —— 合并在 0x8C00~0x10000 的空隙会被填 0xFF，把 NVS/phy_init 抹掉
    $pieces = @(
        @{ offset = '0x0';     name = 'bootloader.bin' },
        @{ offset = '0x8000';  name = 'partition-table.bin' },
        @{ offset = '0x10000'; name = 'ota_data_initial.bin' },
        @{ offset = '0x20000'; name = 'GK87-Screen.bin' }
    )
    foreach ($p in $pieces) {
        $f = Join-Path $Firmware $p.name
        if (-not (Test-Path $f)) { Fail "缺少 $($p.name)，请确认 docs/flasher/firmware/ 完整。" }
        $flashPairs += @($p.offset, $f)
    }
    $sizeMB = [math]::Round((Get-Item (Join-Path $Firmware 'GK87-Screen.bin')).Length / 1MB, 2)
    Write-Host ""
    Write-Host "==> 固件目录：$Firmware" -ForegroundColor Cyan
    Write-Host "    应用镜像 $sizeMB MB + bootloader / 分区表 / otadata"
} else {
    # 兼容单文件合并镜像（注意：它会把 NVS 里的设置一起抹掉）
    $flashPairs = @('0x0', $Firmware)
    $sizeMB = [math]::Round((Get-Item $Firmware).Length / 1MB, 2)
    Write-Host ""
    Write-Host "==> 固件：$Firmware  ($sizeMB MB，合并镜像，会清空设备设置)" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "==> 搜索设备串口" -ForegroundColor Cyan
if ($Port) {
    Write-Host "    使用指定串口 $Port（$(Get-PortLabel $Port)）"
    Show-PortTable $Port
} else {
    for ($i = 1; $i -le 10; $i++) {
        $Port = Find-KeyboardPort
        if ($Port) { break }
        Write-Host "    还没发现串口，等 3 秒（第 $i/10 次）..."
        Start-Sleep -Seconds 3
    }
    if (-not $Port) {
        Fail "没找到任何可用串口。请确认键盘已用 USB 数据线连上、并按键唤醒它。"
    }
    Write-Host "    找到 $Port（$(Get-PortLabel $Port)）"
    Show-PortTable $Port
    Write-Host "    提示：ESP32-S3 会同时暴露两个口 —— 烧录用「USB 串行设备」那个；" -ForegroundColor DarkGray
    Write-Host "          若烧录失败可换另一个（USB JTAG/serial debug unit），或用 -Port 指定。" -ForegroundColor DarkGray
}

if ($Erase) {
    Write-Host ""
    Write-Host "==> 整片擦除" -ForegroundColor Cyan
    & $py.exe @($py.args + @("-m", "esptool", "--chip", "esp32s3", "-p", $Port, "-b", $Baud, "erase_flash"))
    if ($LASTEXITCODE -ne 0) { Fail "擦除失败（端口可能被占用，或键盘休眠了）" }
}

Write-Host ""
Write-Host "==> 烧录到 $Port（$Baud baud）" -ForegroundColor Cyan

# esptool 5.x 把选项名从下划线改成短横线（旧名仍可用但会刷一堆 Deprecated 警告）
$verText = (& $py.exe @($py.args + @("-m", "esptool", "version")) 2>&1 | Out-String)
$major = 4
if ($verText -match '(\d+)\.\d+') { $major = [int]$Matches[1] }
if ($major -ge 5) {
    $before = 'default-reset'; $after = 'hard-reset'; $writeCmd = 'write-flash'
    $optMode = '--flash-mode'; $optFreq = '--flash-freq'; $optSize = '--flash-size'
} else {
    $before = 'default_reset'; $after = 'hard_reset'; $writeCmd = 'write_flash'
    $optMode = '--flash_mode'; $optFreq = '--flash_freq'; $optSize = '--flash_size'
}

$esptoolArgs = @(
    "-m", "esptool", "--chip", "esp32s3", "-p", $Port, "-b", "$Baud",
    "--before", $before, "--after", $after,
    $writeCmd,
    $optMode, "dio", $optFreq, "80m", $optSize, "16MB"
) + $flashPairs
& $py.exe @($py.args + $esptoolArgs)
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "  烧录失败。常见原因：" -ForegroundColor Yellow
    Write-Host "    - 键盘休眠了：按一下键唤醒后重跑"
    Write-Host "    - 串口被占用：关掉串口助手 / idf.py monitor"
    Write-Host "    - 换根 USB 线或换个 USB 口（要走数据线，不是充电线）"
    Write-Host "    - 降速重试：.\tools\flash-noidf.ps1 -Baud 115200"
    exit 1
}

Write-Host ""
Write-Host "  烧录完成，设备已自动重启。" -ForegroundColor Green
Write-Host "  首次使用：屏幕会显示配网二维码，用手机连热点完成配网。"
Write-Host ""
