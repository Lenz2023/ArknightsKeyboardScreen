#!/usr/bin/env bash
# 自动搜索串口并烧录键盘副屏固件（macOS / Linux）
#
# 用法：
#   ./tools/flash.sh              # 自动找串口，编译并烧录
#   ./tools/flash.sh /dev/ttyACM0 # 手动指定串口
#   ./tools/flash.sh -m           # 烧录后打开串口监视
#
# 前置条件：已安装 ESP-IDF 5.1.4，并在当前终端执行过：
#   . $IDF_PATH/export.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PORT=""
MONITOR=0
BAUD=460800

for arg in "$@"; do
  case "$arg" in
    -m|--monitor) MONITOR=1 ;;
    -h|--help)
      sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    /dev/*) PORT="$arg" ;;
    *) echo "未知参数：$arg（用 --help 看用法）" >&2; exit 1 ;;
  esac
done

if ! command -v idf.py >/dev/null 2>&1; then
  cat >&2 <<'EOF'
找不到 idf.py，说明还没导出 ESP-IDF 环境。

请先执行（路径换成你自己的）：
  export IDF_PATH=~/esp/esp-idf-v5.1.4
  . "$IDF_PATH/export.sh"

安装与排错细节见 docs/FLASHING.md
EOF
  exit 1
fi

if [ -z "$PORT" ]; then
  echo "==> 搜索串口"
  # macOS 是 /dev/cu.usbmodem*、/dev/cu.usbserial*；Linux 是 /dev/ttyACM*、/dev/ttyUSB*
  for candidate in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/ttyACM* /dev/ttyUSB*; do
    [ -e "$candidate" ] || continue
    PORT="$candidate"
    break
  done
  if [ -z "$PORT" ]; then
    cat >&2 <<'EOF'
没有找到任何串口设备。

请确认：
  1. 键盘用 USB-C 数据线（不是纯充电线）接到电脑
  2. 拔插一次，让设备重新枚举
  3. macOS 看 ls /dev/cu.*；Linux 看 ls /dev/ttyACM*，并把当前用户加入 dialout 组：
       sudo usermod -aG dialout "$USER"   # 重新登录后生效
EOF
    exit 1
  fi
  echo "    使用 $PORT"
fi

echo ""
echo "==> 编译固件（首次约 5-10 分钟）"
idf.py build

echo ""
echo "==> 烧录到 $PORT（$BAUD baud）"
idf.py -p "$PORT" -b "$BAUD" flash

if [ "$MONITOR" -eq 1 ]; then
  echo ""
  echo "==> 打开串口监视（Ctrl+] 退出）"
  idf.py -p "$PORT" monitor
else
  echo ""
  echo "全部完成。设备已重启，按键 Fn+~ 可在各页面之间切换。"
  echo "需要看日志时运行：idf.py -p $PORT monitor"
fi
