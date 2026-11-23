# 烧录指南

把固件写进键盘副屏，三种方式，按需选一种：

| 方式 | 需要什么 | 适合谁 | 耗时 |
| --- | --- | --- | --- |
| **① 网页刷机** | Chrome / Edge 桌面版 + USB 数据线 | 只想用、不想装环境的人 | 2 分钟 |
| **② 一键脚本（不需要 ESP-IDF）** | Python 3 + USB 数据线 | 想用命令行 / 浏览器不支持 Web Serial | 5 分钟 |
| **③ ESP-IDF 完整环境** | 装 ESP-IDF 5.1.4（约 2GB） | 要改代码重新编译的人 | 30 分钟 |

前提只有一个：**你的键盘是 SKYLOONG GK87 / GK4 4.0（带 320×240 副屏的那款）**。

---

## 方式① 网页刷机（最省事）

仓库里的 `docs/flasher/` 是一个基于 [ESP Web Tools](https://esphome.github.io/esp-web-tools/) 的在线刷机页，
里面已经放好四个镜像（`docs/flasher/firmware/`：bootloader / 分区表 / otadata / 应用，应用约 3.4 MB），
按 `0x0 / 0x8000 / 0x10000 / 0x20000` 分段写入，不会动 NVS——**设备上已保存的 WiFi、森空岛登录、自定义背景都会保留**。

**准备**：桌面版 Chrome 或 Edge（89+）。Firefox / Safari 不支持 Web Serial。

**步骤**：

1. 打开刷机页：**<https://lenz2023.github.io/ArknightsKeyboardScreen/flasher/>**
   （想自己改页面的话，也可以在本机起个服务 —— 必须是 `http://localhost` 或 https，直接双击 html 不行）：
   ```bash
   cd docs/flasher && python -m http.server 8000
   # 浏览器访问 http://localhost:8000
   ```
2. 用**数据线**把键盘连到电脑，点页面上的 **CONNECT**。
3. 弹窗里选新出现的串口（Windows 一般是 `COM6` / `COM7`；macOS 是 `/dev/cu.usbmodem*`）。
   认不准就选名字带 **USB 串行设备** 的那个，。
4. 提示 *Erase device* 时**一般选不要擦除**（保留设置）；只有想恢复出厂才选 Erase。
5. 进度条走完，设备自动重启即完成。

> ⚠️ 键盘闲置约 10 分钟会自动熄屏休眠，休眠时 USB 会断开。**先按一下键唤醒**再点 CONNECT。

**这个页面是怎么来的**（仓库所有者做一次即可）：
`Settings → Pages → Build and deployment → Source: Deploy from a branch → Branch: main / 目录选 /docs → Save`，
等 1 分钟，`docs/flasher/index.html` 就发布在 `https://lenz2023.github.io/ArknightsKeyboardScreen/flasher/`，
`docs/index.html` 则成为站点首页。

---

## 方式② 一键脚本（不需要 ESP-IDF）

只要求系统里有 **Python 3**，脚本会自动装 `esptool`（默认走阿里云镜像）并烧写仓库里的四个镜像。

**Windows（PowerShell）**：

```powershell
cd ArknightsKeyboardScreen
.\tools\flash-noidf.ps1              # 自动找串口 + 烧录
.\tools\flash-noidf.ps1 -Erase       # 恢复出厂：先整片擦除（会清设置）
.\tools\flash-noidf.ps1 -Port COM6   # 手动指定串口
.\tools\flash-noidf.ps1 -Baud 115200 # 烧录不稳时降速
```

**macOS / Linux**：

```bash
cd ArknightsKeyboardScreen
chmod +x tools/flash-noidf.sh
./tools/flash-noidf.sh                # 自动找串口
./tools/flash-noidf.sh -e             # 恢复出厂：先擦除（会清设置）
./tools/flash-noidf.sh -p /dev/ttyACM0
```

脚本会打印每一步做了什么，串口找不到时会每 3 秒重试一次、共 10 次。

---

## 方式③ ESP-IDF 完整环境（要改代码时用）

从零安装编译环境到把固件写进键盘副屏，全部步骤都在这里。

---

## 目录

- [0. 最快路径（已经装好 ESP-IDF 的人）](#0-最快路径已经装好-esp-idf-的人)
- [1. 准备硬件](#1-准备硬件)
- [2. 安装 ESP-IDF 5.1.4](#2-安装-esp-idf-514)
- [3. 获取代码](#3-获取代码)
- [4. 编译](#4-编译)
- [5. 烧录](#5-烧录)
- [6. 用自动脚本（推荐）](#6-用自动脚本推荐)
- [7. 常见报错与解决](#7-常见报错与解决)
- [8. 想完全重来](#8-想完全重来)

---

## 0. 最快路径（已经装好 ESP-IDF 的人）

```bash
# 1. 导出环境
. $IDF_PATH/export.sh            # macOS / Linux
. $env:IDF_PATH\export.ps1       # Windows PowerShell

# 2. 进项目目录
cd ArknightsKeyboardScreen

# 3. 一键：自动找串口 + 编译 + 烧录
./tools/flash.sh                 # macOS / Linux
.\tools\flash.ps1                # Windows
```

脚本会自动跳过蓝牙串口、优先挑 Espressif 的串口，然后编译并烧录。
下面是可以手动的详细版。

---

## 1. 准备硬件

- SKYLOONG GK87 / GK4 键盘（带副屏模块，ESP32-S3，16MB Flash）
- **USB-C 数据线**（必须是能传数据的线；很多充电线只有电源线，插上不会出串口）
- 一台装 Windows 10/11、macOS 或 Linux 的电脑

插上键盘后，系统里应该出现一个 USB 串口：

- Windows：设备管理器 → 端口 →「USB 串行设备 (COM7)」之类
- macOS：`ls /dev/cu.*` 能看到 `cu.usbmodem*`
- Linux：`ls /dev/ttyACM*`

**看不到串口？**

1. 换一根确定能传数据的线（这是最常见的原因）
2. 换一个 USB 口，别用 USB Hub
3. 装驱动：CH343 / CP210x / CH340 驱动（国内厂商常用 CH343）
4. Windows 里看有没有「未知设备」，有就装对应驱动

---

## 2. 安装 ESP-IDF 5.1.4

本项目锁定 **ESP-IDF 5.1.4**（其他版本未验证，LVGL 8.2 与 Arduino 组件都是按 5.1.4 配的）。

### 方式 A：官方安装器 / VSCode 插件（新手推荐）

1. 装 [VSCode](https://code.visualstudio.com/) + 「Espressif IDF」扩展
2. 扩展里选 `ESP-IDF v5.1.4` 安装，安装器会自动装 Python 环境与工具链
3. 装完在 VSCode 底部状态栏点「ESP-IDF Terminal」

### 方式 B：命令行安装

```bash
git clone -b v5.1.4 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.1.4
cd esp-idf-v5.1.4
./install.sh esp32s3            # Windows: .\install.ps1 esp32s3
```

GitHub 访问慢时可用国内镜像 `https://dl.espressif.cn/github_assets/espressif/esp-idf/releases/download/v5.1.4/esp-idf-v5.1.4.zip`。

> 只需要 `esp32s3` 这一个 target，能省不少时间和磁盘。

### 每次开新终端都要导出环境

```bash
# macOS / Linux
export IDF_PATH=~/esp/esp-idf-v5.1.4
. "$IDF_PATH/export.sh"

# Windows PowerShell
$env:IDF_PATH = "D:\esp\esp-idf-v5.1.4"
. "$env:IDF_PATH\export.ps1"
```

验证：

```bash
idf.py --version
```

### 机器上有多个 Python 时的坑

`export.ps1` 会用 PATH 里的 python 去猜虚拟环境目录。如果它报：

```
ERROR: ...\python_env\idf5.1_py3.10_env\Scripts\python.exe doesn't exist!
Cannot import module "click"
```

说明它猜错了版本（实际装的是 3.11 的环境）。显式指定即可：

```powershell
$env:IDF_TOOLS_PATH      = "D:\esp\.espressif"          # 你装工具链的位置
$env:IDF_PYTHON_ENV_PATH = "$env:IDF_TOOLS_PATH\python_env\idf5.1_py3.11_env"
. "$env:IDF_PATH\export.ps1"
```

---

## 3. 获取代码

```bash
git clone <本仓库地址> ArknightsKeyboardScreen
cd ArknightsKeyboardScreen
```

仓库里**已经包含** `components/` 下的所有依赖（Arduino、LVGL、TFT_eSPI、
esp_littlefs、lodepng 等）与 `sdkconfig`，所以**不需要联网拉组件**。

> `managed_components/` 与 `build/` 不在仓库里，第一次编译会自动生成。

---

## 4. 编译

```bash
idf.py build
```

- 第一次约 **5–10 分钟**（会编译整个 LVGL 与 Arduino 组件）
- 之后改 `main/` 里的文件只需 **30–60 秒**（增量编译）
- 成功标志：出现 `Project build complete`，并打印 `GK87-Screen.bin binary size 0x36xxxx bytes`

### 需要 `idf.py reconfigure` 的情况

`main/CMakeLists.txt` 用 `file(GLOB ...)` 收集源文件，**只在 CMake 配置期求值**。
所以下面几种操作后要手动 reconfigure，否则会出现「文件不存在」或「符号未定义」：

- 新增或删除了 `main/` 下的 `.c` / `.cpp` 文件
- 从上游旧版本切过来（上游有 `heavy_rain_home_bg.c`，本版换成了 `scene_home_bg.c`）

```bash
idf.py reconfigure && idf.py build
```

---

## 5. 烧录

先查串口号（Windows）：

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

或者看设备管理器。**注意别选成蓝牙串口**（名字里带「蓝牙链接上的标准串行」）。

```bash
idf.py -p COM7 -b 460800 flash
# macOS:  idf.py -p /dev/cu.usbmodem1234 -b 460800 flash
# Linux:  idf.py -p /dev/ttyACM0 -b 460800 flash
```

成功的标志：

```
Hash of data verified.        ← 会出现 4 次（bootloader / 分区表 / ota_data / 应用）
Hard resetting via RTS pin...
```

设备会自动重启进入新固件。

想顺便看日志（`Ctrl+]` 退出）：

```bash
idf.py -p COM7 monitor
```

烧录不稳定时把波特率降到 115200：

```bash
idf.py -p COM7 -b 115200 flash
```

---

## 6. 用自动脚本（推荐）

`tools/flash.ps1`（Windows）与 `tools/flash.sh`（macOS / Linux）会：

1. 检查 `idf.py` 是否在 PATH 里，没有就打印该怎么导出环境
2. 自动枚举串口，**跳过蓝牙串口**，优先选 Espressif VID_303A 的设备
3. 编译（可 `-NoBuild` 跳过）
4. 烧录（可指定 `-Port` / `-Baud`）
5. 可选直接打开串口监视（`-Monitor`）

```powershell
# Windows
.\tools\flash.ps1
.\tools\flash.ps1 -Port COM7 -Monitor
.\tools\flash.ps1 -Baud 115200          # 烧录不稳时
.\tools\flash.ps1 -NoBuild              # 只烧，不编
```

```bash
# macOS / Linux
./tools/flash.sh
./tools/flash.sh /dev/ttyACM0 -m
```

> Linux 上如果提示权限不足：`sudo usermod -aG dialout $USER`，重新登录生效。

---

## 7. 常见报错与解决

### `UnicodeDecodeError: 'gbk' codec can't decode byte 0x82`

这**不是编码问题**，而是 esptool 的中文报错被 idf.py 用 GBK 解码失败，
真正的错误被吞掉了。**99% 的情况是串口不存在了**（板子掉线、线松了、端口号变了）。

处理：重新 `GetPortNames()` 确认端口号，拔插一次 USB，再烧。

### `could not open port 'COM7': FileNotFoundError` / `PermissionError`

- 端口号变了（设备重新枚举后会换号）
- 串口被占用：关掉串口助手、另一个 `idf.py monitor`、Arduino IDE 等
- Linux：权限不够，见上面 dialout

### `cc1.exe: fatal error: main/xxx.c: No such file or directory`

`file(GLOB ...)` 的缓存问题。跑 `idf.py reconfigure` 再编译。

### `ninja: error: loading 'build.ninja': No such file or directory`

第一次编译前没有生成构建目录，直接 `idf.py build` 即可；或者删掉 `build/` 重新来。

### 编译报 `format-truncation` / `-Werror` 相关错误

本项目 `main/CMakeLists.txt` 里对几个已知无害的警告做了降级（`-Werror=all` 但放行
`stringop-truncation` 等）。如果你自己加了新代码触发别的警告，按提示改代码即可，
不建议直接关掉 `-Werror`。

### 烧录成功但屏幕没反应

- 断电重插（不是重新烧录）
- 检查是不是烧到了旧的 `GK87-Screen.bin`：`idf.py build` 输出的 size 应与
  `build/GK87-Screen.bin` 一致
- 看 `idf.py monitor` 有没有 panic 输出

### `WARNING: Git was not found`

无害，只影响版本号显示。想消掉就装个 Git。

---

## 8. 想完全重来

```bash
rm -rf build sdkconfig.old          # 清构建产物（Windows: Remove-Item -Recurse -Force build）
idf.py reconfigure
idf.py build
idf.py -p COM7 -b 460800 flash
```

想恢复出厂设置：长按 `Fn+Menu` 进设置，或在 Web 配置页调用重启接口后重新配网。
设备里的用户图片保存在 LittleFS，重烧应用不会清掉；要清可以进图库页面逐个「恢复默认」。
