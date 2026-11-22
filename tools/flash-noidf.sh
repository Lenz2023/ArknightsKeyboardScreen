#!/usr/bin/env bash
# 不依赖 ESP-IDF 的烧录脚本（macOS / Linux）
#
# 用法：
#   ./tools/flash-noidf.sh                 # 自动找串口并烧录
#   ./tools/flash-noidf.sh -p /dev/ttyACM0 # 指定串口
#   ./tools/flash-noidf.sh -e              # 先整片擦除再烧（首次安装推荐）
#   ./tools/flash-noidf.sh -f path/to/firmware.bin
#
# 只需要系统里有 python3（脚本会自动装 esptool）。

set -u

PORT=""
BAUD=460800
FIRMWARE=""
ERASE=0
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  exit 0
}

while getopts "p:b:f:eh" opt; do
  case "$opt" in
    p) PORT="$OPTARG" ;;
    b) BAUD="$OPTARG" ;;
    f) FIRMWARE="$OPTARG" ;;
    e) ERASE=1 ;;
    h) usage ;;
    *) usage ;;
  esac
done

fail() {
  printf '\n  [错误] %s\n\n' "$1" >&2
  exit 1
}

echo
echo "==> 检查 Python"
PY=""
for candidate in python3 python; do
  if command -v "$candidate" >/dev/null 2>&1; then
    if "$candidate" -c 'import sys; sys.exit(0 if sys.version_info[0] == 3 else 1)' 2>/dev/null; then
      PY="$candidate"
      break
    fi
  fi
done
[ -n "$PY" ] || fail "没找到 python3。请先安装 Python 3，或用网页版刷机：docs/flasher/"
echo "    $PY ($($PY --version 2>&1))"

echo
echo "==> 检查 esptool"
if ! "$PY" -m esptool version >/dev/null 2>&1; then
  echo "    未安装，正在安装 esptool ..."
  "$PY" -m pip install --upgrade esptool || fail "esptool 安装失败，可手动执行：pip3 install esptool"
fi
echo "    已就绪"

if [ -z "$FIRMWARE" ]; then
  FIRMWARE="$REPO_ROOT/docs/flasher/firmware"
fi
[ -e "$FIRMWARE" ] || fail "找不到固件：$FIRMWARE"

# 四分片写，不做合并：合并在 0x8C00~0x10000 的空隙会被填 0xFF，把 NVS/phy_init 抹掉
if [ -d "$FIRMWARE" ]; then
  for f in bootloader.bin partition-table.bin ota_data_initial.bin GK87-Screen.bin; do
    [ -f "$FIRMWARE/$f" ] || fail "缺少 $f，请确认 docs/flasher/firmware/ 完整。"
  done
  FLASH_PAIRS=(0x0 "$FIRMWARE/bootloader.bin" 0x8000 "$FIRMWARE/partition-table.bin" 0x10000 "$FIRMWARE/ota_data_initial.bin" 0x20000 "$FIRMWARE/GK87-Screen.bin")
else
  FLASH_PAIRS=(0x0 "$FIRMWARE")
fi
echo
echo "==> 固件：$FIRMWARE  ($(du -h "$FIRMWARE" | cut -f1))"

echo
echo "==> 搜索设备串口"
if [ -z "$PORT" ]; then
  for _ in 1 2 3 4 5 6 7 8 9 10; do
    for pattern in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/ttyACM* /dev/ttyUSB*; do
      [ -e "$pattern" ] && PORT="$pattern" && break
    done
    [ -n "$PORT" ] && break
    echo "    还没发现串口，等 3 秒 ..."
    sleep 3
  done
  [ -n "$PORT" ] || fail "没找到串口。请确认键盘已用 USB 数据线连上、并按键唤醒它。"
  echo "    找到 $PORT"
  if [ -d /dev/serial/by-id ]; then
    echo "    系统里的串口设备名（更好认）："
    ls -l /dev/serial/by-id 2>/dev/null | sed -n 's#.*-> \(.*\)#      \1#p'
  fi
  echo "    提示：ESP32-S3 会同时暴露两个口，选 usbmodem / ttyACM 那个；不行就换另一个。"
else
  echo "    使用指定串口 $PORT"
fi

if [ "$ERASE" = "1" ]; then
  echo
  echo "==> 整片擦除"
  "$PY" -m esptool --chip esp32s3 -p "$PORT" -b "$BAUD" erase_flash || fail "擦除失败"
fi

echo
echo "==> 烧录到 $PORT（$BAUD baud）"
"$PY" -m esptool --chip esp32s3 -p "$PORT" -b "$BAUD" \
  --before default_reset --after hard_reset \
  write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB \
  "${FLASH_PAIRS[@]}" || {
    echo
    echo "  烧录失败。常见原因：" >&2
    echo "    - 键盘休眠了：按一下键唤醒后重跑" >&2
    echo "    - 串口被占用：关掉串口助手 / idf.py monitor" >&2
    echo "    - 换根 USB 数据线或换个 USB 口" >&2
    echo "    - 降速重试：$0 -b 115200" >&2
    exit 1
  }

echo
echo "  烧录完成，设备已自动重启。"
echo "  首次使用：屏幕会显示配网二维码，用手机连热点完成配网。"
echo
