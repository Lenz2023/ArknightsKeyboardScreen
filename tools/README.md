# tools/ — 烧录脚本与资源生成

这里有两类东西：

1. **一键烧录脚本**（`flash.ps1` / `flash.sh`）——自动找串口、编译、烧录，日常用这个；
2. **资源生成脚本**——从原始素材重新生成固件里内嵌的图片资源。

## 环境

- 烧录脚本：需要已安装 ESP-IDF 5.1.4 并导出过环境（见 `docs/FLASHING.md`）
- 生成脚本：`pip install pillow`

---

# 一键烧录

分两组：**不需要 ESP-IDF** 的（普通用户）和**需要 ESP-IDF** 的（改代码的人）。

## 不需要 ESP-IDF（烧仓库里现成的固件）

| 脚本 | 平台 | 用法 |
| --- | --- | --- |
| `flash-noidf.ps1` | Windows PowerShell | `.\tools\flash-noidf.ps1` |
| `flash-noidf.sh` | macOS / Linux | `./tools/flash-noidf.sh` |

只要求系统有 **Python 3**：脚本会自动 `pip install esptool`，然后烧
`docs/flasher/firmware/` 里的四个镜像到 `0x0 / 0x8000 / 0x10000 / 0x20000`——**不动 NVS**，已保存的设置会保留。

```powershell
.\tools\flash-noidf.ps1                  # 自动找串口
.\tools\flash-noidf.ps1 -Erase           # 恢复出厂：先整片擦除（会清设置）
.\tools\flash-noidf.ps1 -Port COM6       # 指定串口
.\tools\flash-noidf.ps1 -Baud 115200     # 烧录不稳时降速
```

```bash
./tools/flash-noidf.sh                   # 自动找串口
./tools/flash-noidf.sh -e                # 恢复出厂：先擦除（会清设置）
./tools/flash-noidf.sh -p /dev/ttyACM0
```

> 还有更省事的方式：直接开 `docs/flasher/index.html`（需 https 或 localhost），用浏览器 Web Serial 刷机，连 Python 都不用。

## 需要 ESP-IDF（自己编译）

| 脚本 | 平台 | 用法 |
| --- | --- | --- |
| `flash.ps1` | Windows PowerShell | `.\tools\flash.ps1` |
| `flash.sh` | macOS / Linux | `./tools/flash.sh` |

编译 + 烧录；自动枚举串口并跳过蓝牙串口（优先 Espressif VID_303A 设备）。

```powershell
.\tools\flash.ps1                    # 自动找口 + 编译 + 烧录
.\tools\flash.ps1 -Port COM7 -Monitor
.\tools\flash.ps1 -Baud 115200
.\tools\flash.ps1 -NoBuild           # 只烧不编
```

```bash
./tools/flash.sh                     # 自动找口
./tools/flash.sh /dev/ttyACM0 -m     # 指定口 + 烧完看日志
```

---

## 更新刷机用固件（改完代码后）

`docs/flasher/firmware/` 里放的是四个镜像，编译完直接覆盖：

```powershell
Copy-Item build\bootloader\bootloader.bin        docs\flasher\firmware\ -Force
Copy-Item build\partition_table\partition-table.bin docs\flasher\firmware\ -Force
Copy-Item build\ota_data_initial.bin              docs\flasher\firmware\ -Force
Copy-Item build\GK87-Screen.bin                   docs\flasher\firmware\ -Force
```

```bash
cp build/bootloader/bootloader.bin build/partition_table/partition-table.bin \
   build/ota_data_initial.bin build/GK87-Screen.bin docs/flasher/firmware/
```

**不要**把它们 `merge_bin` 成一个文件：合并会把 `0x8C00~0x10000`（NVS + phy_init）填成 `0xFF`，
用户烧完就丢了 WiFi 和登录状态。

---

# 资源生成

脚本默认路径都相对仓库根目录，在仓库任意位置执行即可。

## 脚本

### `gen_scene_bg.py` → `main/scene_home_bg.c`

把 `tools/scene-src/` 里的 6 张原始场景 PNG（1365×1024）缩放为 320×240 的 RGB565，
生成 C 数组文件 `main/scene_home_bg.c`。

| 输出符号 | 源文件 | 对应场景 | 触发条件 |
|---|---|---|---|
| `scene_clear_bg` | `shiyie.png` | 视野（罗德岛顶层甲板，晴日） | 白天 · 晴朗 |
| `scene_cloudy_bg` | `yinyun.png` | 阴云（雨雾之城） | 白天 · 多云 |
| `scene_rain_bg` | `mihai.png` | 秘海（风雨海面） | 白天 · 雨天 |
| `scene_snow_bg` | `yandong.png` | 严冬（风雪荒村） | 白天 · 降雪 |
| `scene_dusk_bg` | `shangye.png` | 商业（暮色霓虹街景） | 17:00–20:00 / 05:00–06:00 |
| `scene_night_bg` | `yemu.png` | 夜幕（罗德岛双月夜空） | 20:00–05:00 |

```bash
python tools/gen_scene_bg.py
```

### `verify_scene_bg.py`

反向校验：从 `main/scene_home_bg.c` 解析出 RGB565 数组，还原成 PNG 并拼成对照图，
用来确认资源字节没有写坏（生成后建议跑一次）。

```bash
python tools/verify_scene_bg.py     # 输出到 tools/out/
```

### `gen_garrison_emblem.py`

从 PRTS 的「卫戍协议_Logo_协防干员」图标 alpha 通道生成
`main/apps/GarrisonEmblem.inc`（`garrisonBadge` 21×24 与 `garrisonEmblem` 104×116 两张 A8 图）。

输入图需自行准备为 `tools/logo_coop.png`（58×64 RGBA）。来源页面：
<https://prts.wiki/w/卫戍协议>

```bash
python tools/gen_garrison_emblem.py
```

## 素材来源与版权

`tools/scene-src/` 中的场景图取自 [PRTS 明日方舟维基 · 首页场景一览](https://prts.wiki/w/首页场景一览)，
著作权归 **HYPERGRYPH（鹰角网络）** 所有。此处仅作个人学习与非商业展示用途，
**不随本仓库的 MIT 许可再分发**，详见仓库根目录 `LICENSE-THIRD-PARTY.md`。
