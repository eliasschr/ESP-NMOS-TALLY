#!/usr/bin/env bash
# Build a complete 4 MiB ESP32 flash image for Wokwi. The filesystem offset is
# read from the active partition table instead of being duplicated here.
set -euo pipefail

readonly environment="esp32dev"
readonly build_dir=".pio/build/${environment}"
readonly partition_file="partitions_wokwi.csv"
readonly output="dist/esp-nmos-tally-wokwi.bin"
readonly flash_size=$((4 * 1024 * 1024))

require_command() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "ERROR: Required command not found: $1" >&2
    exit 1
  }
}

require_file() {
  [[ -s "$1" ]] || {
    echo "ERROR: Required non-empty file not found: $1" >&2
    exit 1
  }
}

require_command pio
require_command python3
require_file "$partition_file"

pio run -e "$environment"
pio run -e "$environment" -t buildfs

readonly firmware="${build_dir}/firmware.bin"
readonly bootloader="${build_dir}/bootloader.bin"
readonly partitions="${build_dir}/partitions.bin"
readonly filesystem="${build_dir}/littlefs.bin"
readonly pio_home="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"
readonly esptool="${pio_home}/packages/tool-esptoolpy/esptool.py"
# PlatformIO supplies ESP32 boot metadata with the Arduino framework, not the
# esptool package. Keep this path tied to the installed framework package.
readonly boot_app0="${pio_home}/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

for file in "$firmware" "$bootloader" "$partitions" "$filesystem" "$esptool" "$boot_app0"; do
  require_file "$file"
done

readonly littlefs_offset="$(python3 - "$partition_file" <<'PY'
import csv
import sys

with open(sys.argv[1], newline='', encoding='utf-8') as source:
    for row in csv.reader(source):
        if not row or row[0].strip().startswith('#'):
            continue
        if row[0].strip() == 'littlefs':
            print(int(row[3].strip(), 0))
            break
    else:
        raise SystemExit('ERROR: littlefs partition is missing')
PY
)"

mkdir -p dist
python3 "$esptool" --chip esp32 merge_bin -o "$output" \
  --flash_size 4MB \
  0x1000 "$bootloader" \
  0x8000 "$partitions" \
  0xe000 "$boot_app0" \
  0x10000 "$firmware" \
  "$littlefs_offset" "$filesystem"

python3 - "$output" "$flash_size" "$littlefs_offset" "$bootloader" "$partitions" "$boot_app0" "$firmware" "$filesystem" <<'PY'
import os
import sys

output, flash_size, filesystem_offset, *images = sys.argv[1:]
flash_size = int(flash_size)
filesystem_offset = int(filesystem_offset)
offsets = [0x1000, 0x8000, 0xe000, 0x10000, filesystem_offset]
regions = sorted((offset, offset + os.path.getsize(path), path) for offset, path in zip(offsets, images))

if os.path.getsize(output) == 0:
    raise SystemExit('ERROR: merged image is empty')
if os.path.getsize(output) > flash_size:
    raise SystemExit('ERROR: merged image exceeds the configured 4 MiB flash')
for previous, current in zip(regions, regions[1:]):
    if previous[1] > current[0]:
        raise SystemExit(f'ERROR: overlapping regions: {previous[2]} and {current[2]}')
if regions[-1][1] > flash_size:
    raise SystemExit('ERROR: a flash component exceeds the configured 4 MiB flash')

print(f'Validated {output}: {os.path.getsize(output)} bytes; LittleFS at 0x{filesystem_offset:x}.')
PY

echo "Wokwi image ready: $output"
