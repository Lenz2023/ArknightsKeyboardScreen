# 第三方组件许可证清单

本仓库包含多个第三方开源组件。**它们各自保留原有许可证**，不受根目录 `LICENSE`（MIT）或上游 Skyloong 授权条款影响。
分发本项目时请一并保留以下许可证文件。

| 组件 | 路径 | 许可证 | 许可证文件 |
|---|---|---|---|
| Arduino-ESP32 | `components/arduino/` | LGPL-2.1 | [`components/arduino/LICENSE`](components/arduino/LICENSE) |
| LVGL | `components/lvgl/` | MIT | [`components/lvgl/LICENCE.txt`](components/lvgl/LICENCE.txt) |
| TFT_eSPI | `components/TFT_eSPI/` | FreeBSD（含部分字体/驱动子许可） | [`components/TFT_eSPI/license.txt`](components/TFT_eSPI/license.txt) |
| ESP32-audioI2S / es8311 驱动 | `components/es8311/` | Apache-2.0 | [`components/es8311/LICENSE`](components/es8311/LICENSE) |
| esp_littlefs（含 littlefs） | `components/esp_littlefs/` | MIT | [`components/esp_littlefs/LICENSE.md`](components/esp_littlefs/LICENSE.md) |
| cJSON | 随 ESP-IDF / 组件引入 | MIT | — |
| lodepng | 随 LVGL `src/extra/libs/png` 引入 | zlib | 见文件头声明 |
| mbedTLS | ESP-IDF 内置 | Apache-2.0 | — |
| QRCode（LVGL 扩展） | 随 LVGL `src/extra/libs/qrcode` 引入 | MIT | 见文件头声明 |

`components/arduino/` 与 `components/lvgl/` 内部子目录（如 `libb64`、`BLE`、`esp_littlefs/src/littlefs`）
还各自带有许可证文件，均保留在原位。

---

## 关于素材

`ui-design/`、`keytone/`、`web/` 中的图片、音频与字体属于**素材资源**，不代表可自由再分发：

- 森空岛 / 明日方舟相关素材（`main/apps/Doctor*.inc`、`main/apps/GarrisonEmblem.inc`、`main/apps/OperatorNames.inc`、`main/scene_home_bg.c` 等）
  的著作权归 **HYPERGRYPH（鹰角网络）** 所有，本项目仅在个人非商业场景下使用，**不随本仓库授权再分发**。
- 首页场景图片来源为 [PRTS 明日方舟维基](https://prts.wiki/w/首页场景一览)，供个人学习使用。
- 干员英文名（appellation）数据来自公开的游戏数据表，著作权归鹰角网络所有。
- `keytone/` 中的按键音效与 `web/` 中的界面图标来源各异，如需商用请自行确认授权。

如需商业使用，请自行取得鹰角网络与相应素材权利方的授权。
