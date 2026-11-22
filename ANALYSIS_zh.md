# ESP32 Screen Module 项目分析

> 分析对象：`idf-v5.1.4` 分支，仓库 `JZ-Skyloong/esp32_screen_module`。本报告根据源码静态阅读整理，未进行固件编译或硬件运行验证。

## 1. 项目定位

这是 SKYLOONG GK87 屏幕模块的 ESP32-S3 固件。模块作为键盘上的独立显示设备，通过 UART 与键盘主控交换状态与控制事件；本机运行 LVGL UI，并可通过 Wi-Fi 联网、内置网页服务器和 LittleFS 管理内容。屏幕分辨率在 `main/include/A_Config.h` 中定义为 320×240，固件版本宏为 `SCM_V4.0.0`。

主要功能包括主页（时间、键盘状态、电池等）、APS 按键速度统计、GIF/MPEG 与 JPG 展示、天气、电脑 CPU/内存状态、设置及二维码配网界面。还有开机 MPEG 动画、按键音、RTC、亮度/音量与休眠管理。

## 2. 目录结构

- `main/`：产品固件。`main.cpp` 是启动入口；`hal.cpp` 集中硬件抽象、显示/音频、系统事件队列和 WebServer；`protocol.cpp` 处理键盘串口协议；`apps/` 存放界面应用；`esp_dpp_prov.cpp` 提供 Wi-Fi 配网 UI；`internationalization.cpp` 管理语言和时区；`imgs/`、`weather/`、`img2/`、字体及 `boot_animation.h` 是编译进固件的图形资源。
- `components/`：随仓库提交的依赖，包括 Arduino-ESP32、LVGL、TFT_eSPI、LittleFS、ES8311 codec 驱动等。依赖直接 vendoring，因此仓库文件量大。
- `web/`：配套网页源文件和图片/JS/CSS。`updateWeb.sh` 把静态资源 gzip 后转成 C 头文件，供固件通过 HTTP 从 Flash 提供。
- `keytone/`、`main/keytone/`：按键音资源及对应 C 数组头文件。
- `sdkconfig`：ESP-IDF 配置；`partition_table.csv`：Flash 分区；根 `CMakeLists.txt` 和 `main/CMakeLists.txt`：ESP-IDF 构建入口。
- `createBin.sh`：将 bootloader、分区表、OTA 数据和应用合并为烧录镜像。

## 3. 启动和任务模型

`app_main()` 在 `main/main.cpp` 中执行：初始化 Arduino 和 PSRAM，调用 `hal.init()` 初始化硬件、文件系统与 LVGL，读取 RTC 时间并初始化 Wi-Fi 凭据；创建各个 App 对象并交给 `AppManagerLite`。若发生冷启动且配置允许，则播放内置 MPEG 启动动画；随后启动 UART 协议、LVGL 定时器处理任务和 USB 调试输入任务。已有 Wi-Fi 凭据时恢复上次 App，否则进入二维码配网模式。

`HAL::init()` 在 `main/hal.cpp` 中初始化屏幕背光/电源、TFT_eSPI、ES8311 音频 codec 与 I2S、DS1302 RTC、LittleFS、NVS Preferences、LVGL 显示驱动和键盘输入设备。两个 LVGL 绘制缓冲放在 PSRAM。系统控制任务消费事件队列；App 管理器另有界面循环和切换任务。LVGL 访问通过 HAL 锁保护，App 切换时先停止/销毁旧页面再创建新页面。

## 4. 主要模块与数据流

### 硬件抽象和键盘协议

`main/include/A_Config.h` 定义了显示电源/背光、RTC、UART、I2S 和 codec I2C 引脚，以及屏幕尺寸、音频采样率等硬件参数。`main/include/hal.h` 定义键盘无线/蓝牙/USB 通道、锁定键状态、电池状态以及系统事件。

`protocol.cpp` 使用 UART1（115200，RX=17、TX=1）与键盘通信。帧结构为 STX、长度、类型、数据、ETX 和 XOR CRC；协议处理键盘状态、电池、时间、显示控制、设置/二维码切换和按键音事件。键盘按下事件会触发预置或从 LittleFS 读取的 WAV/MP3 音频。超过约 10 分钟没有通信且当前为非 USB 通道时，电源任务进入休眠流程。

### App 页面

`main/include/APP_Def.hpp` 定义 App ID：主页 1、APS 2、GIF 3、JPG 4、天气 5、系统信息 6、设置 50、二维码 100。`AppManagerLite` 按顺序切换页面，并允许在配置中禁用部分页面；最近使用的页面 ID 持久化保存。

- `home.cpp`：主页和键盘/电池状态 UI。
- `APS.cpp`：统计按键速率和总数，并绘制图表。
- `GIF.cpp`：从 LittleFS 枚举 `.mpeg` 文件并通过 MPEG 解码播放。
- `jpg.cpp`：从 LittleFS 枚举 JPG，按固定文件或滚动模式显示。
- `Weather.cpp`：查询心知天气的实时天气、生活建议和 3 天天气预报，图标资源编译在固件中。
- `Sysinfo.cpp`：连接配置的局域网 IP/端口，接收 `float cpu_pct; float mem_pct` 结构并绘制仪表。
- `settings.cpp`：配置网络、时间、页面开关、主题、亮度、音量、语言、按键音、网页服务器、OTA/恢复等。
- `qrcode.cpp` 与 `esp_dpp_prov.cpp`：显示并处理 Wi-Fi Easy Connect/DPP 配网流程。

### Web 配置端和文件系统

网页服务器在 `HAL::start_webserver()` 中启动。若设备没有可用 STA 连接，则启动名为 `SKYLOONG 4.0 Screen` 的 SoftAP，使用 192.168.4.1 并运行 DNS 捕获门户；否则通过已连接的局域网提供服务。接口涵盖设备信息、扫描 Wi-Fi、配置 Wi-Fi/主题/时区/语言/按键音和页面开关、配置 JSON、时间同步、重启，以及 LittleFS 文件列表、上传、创建和删除。上传 JPG/MPEG 会请求对应 App 刷新。静态网页资源由 `web/` 与 `main/include/webserver/` 组合提供。

网络与持久化数据分成几类：NVS Preferences 保存屏幕设置和最近页面；LittleFS 的 `/.wifi.csv` 保存 Wi-Fi 名称/密码，`/.cfg.bin` 保存 App 配置结构，媒体文件保存在 LittleFS。天气配置为心知 API key 和城市；系统信息配置为电脑 IP、端口及自定义文字。

## 5. 构建与 Flash 布局

README 指定 ESP-IDF 5.1.4、VS Code ESP-IDF 扩展和 Windows 编译环境，常规命令是 `idf.py build`，通过 `idf.py -p PORT flash` 烧录。根 CMake 使用 `$IDF_PATH/tools/cmake/project.cmake`，工程名 `GK87-Screen`；`main/CMakeLists.txt` 递归收集 `.c/.cpp` 文件。

当前分支 `sdkconfig` 目标为 ESP32-S3、16 MB Flash、自定义分区表。分区表包括 NVS、PHY、OTA 元数据、两个各 5 MB OTA 应用槽，以及约 5.8 MB 的 SPIFFS/LittleFS 区域（分区名为 `spiffs`，项目代码通过 LittleFS API 使用）。因此这套布局对应 16 MB Flash 硬件。

## 6. 分支差异

查询仓库远端时有四个分支；下面的分支头提交时间来自仓库当前远端快照（2026-11-14 检查）。

| 分支 | 头提交 | Flash/分区 | 主要用途与差别 |
|---|---|---|---|
| `master` | `a6da5bb`，2024-06-21 | sdkconfig 16 MB；两个 4 MB OTA 槽，较大的文件系统分区 | README 保留 ESP-IDF 4.4.7 和 macOS/Linux 编译说明，是较早的主线版本。 |
| `16m_flash` | `e855602`，2024-06-14 | 16 MB；与 master 相同的 4 MB OTA 分区和文件系统布局 | 提交说明是“重新适配16M Flash”。与 master 的已跟踪差异主要是 README 内容被大幅缩减，应用源码差异很少。 |
| `extend_32M_flash` | `759df1f`，2024-06-13 | sdkconfig 32 MB；两个 4 MB OTA 槽，文件系统扩大到约 23.8 MB | 面向 32 MB Flash 硬件，便于存储更多用户媒体文件。与 `16m_flash` 的业务源码基本一致，差异集中在 sdkconfig 与分区表。 |
| `idf-v5.1.4` | `93369ec`，2026-05-29 | sdkconfig 16 MB；两个 5 MB OTA 槽和约 5.8 MB 文件系统 | 此次分析对象。README 指定 ESP-IDF 5.1.4 和 Windows；源码及依赖配置相较 2024 分支有较大演进，并调整了分区以容纳更大的应用固件。 |

旧分支间 flash 容量差异主要体现为 `sdkconfig` 与 `partition_table.csv`；`16m_flash` 和 `extend_32M_flash` 在选定核心固件文件上的差异只有分区表和 sdkconfig。`idf-v5.1.4` 相对旧分支的 diff 数字看起来很大，原因还包括提交历史并非相同基线、TFT_eSPI/Arduino/LVGL 等 vendored 依赖的增删，不宜把变更文件总数直接解读为产品功能重写。

## 7. 静态审阅发现

以下是源码审阅发现，未通过构建或硬件测试确认：

1. **合并镜像脚本与当前配置的 Flash 容量不一致。** `sdkconfig` 选择 16 MB，`partition_table.csv` 也按约 16 MB 布局，但 `createBin.sh` 的 `esptool.py merge_bin` 参数固定为 `--flash_size 32MB`。这会造成镜像头/目标容量描述不一致，发布前应核对目标板实际 Flash 容量并统一参数。
2. **Web 配置与文件管理接口没有鉴权。** `hal.cpp` 启动开放 SoftAP，且 `server.enableCORS(true)`；源码未见认证校验。Web 接口可以上传/删除文件、修改配置和重启设备。若服务器运行在共享 LAN 上，局域网内其他用户可能调用这些接口。
3. **若干请求值直接复制到固定长度缓冲区。** 如 `/config_wifi` 将请求参数 `strcpy` 到 64 字节的 `hal.ssid`、`hal.password`；`/config_keytone`、`/config_app_jpg` 也有固定数组写入。未经长度检查的过长参数可能覆盖内存，应改为有界复制并验证长度。
4. **配置 JSON 和天气 JSON 解析缺少完整的空值/长度校验。** `parseAppSettings()` 在解析失败后回退默认 JSON，但随后直接解引用对象字段并复制到定长数组；天气 API 的 JSON 也直接解引用字段并 `strcpy`。错误响应或异常字段可能导致空指针访问或缓冲区越界。
5. **Wi-Fi 凭据以明文写入文件系统。** `/.wifi.csv` 以 `SSID,password` 文本格式保存。设备文件系统或固件调试访问可读取凭据；文件列表接口会过滤点文件，但这不等同于对数据本身加密。
6. **天气和 OTA URL 使用 HTTP。** `Weather.cpp` 访问心知 API 的 URL 是 `http://`；设置页 OTA 更新地址也为 HTTP。传输内容缺少 TLS 保护，网络中间人可观察或修改传输数据。
7. **分支文档与脚本需进一步确认。** README 写本分支只支持 Windows，但 `createBin.sh` 是 Bash 脚本；该脚本或许只供特定发布环境使用。README 也没有补充 Web API、配网和媒体格式细节。

## 8. 总结

`idf-v5.1.4` 是一个将 Arduino-ESP32、ESP-IDF、LVGL 和 TFT_eSPI 混合使用的 ESP32-S3 产品固件；核心设计为 FreeRTOS 多任务、LVGL 单屏 UI、UART 键盘协议、Wi-Fi/HTTP 配套配置端和 LittleFS 媒体存储。当前分支适配 16 MB Flash，并支持双 OTA 槽。最需要优先核对的是合并镜像脚本的 32 MB 参数与固件 16 MB 配置，以及开放 Web 配置接口的访问控制和请求长度校验。
