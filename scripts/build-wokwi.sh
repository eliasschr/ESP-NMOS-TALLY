#!/usr/bin/env bash
# Build a complete 4 MiB ESP32 flash image for Wokwi. The partition binary is
# decoded after the build and compared with both the CSV and merged image.
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

# PlatformIO's esptool package imports pyserial from PlatformIO Core's Python
# environment. Running it with the shell's Python can select an unrelated
# interpreter without that dependency.
readonly pio_executable="$(command -v pio)"
readonly pio_python="$(sed -n '1s/^#!//p' "$pio_executable")"
[[ -x "$pio_python" ]] || {
  echo "ERROR: Could not determine PlatformIO's Python interpreter" >&2
  exit 1
}

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

readonly spiffs_offset="$(python3 - "$partition_file" <<'PY'
import csv
import sys

with open(sys.argv[1], newline='', encoding='utf-8') as source:
    for row in csv.reader(source):
        if not row or row[0].strip().startswith('#'):
            continue
        if row[0].strip() == 'spiffs':
            print(int(row[3].strip(), 0))
            break
    else:
        raise SystemExit('ERROR: spiffs partition is missing')
PY
)"

mkdir -p dist
"$pio_python" "$esptool" --chip esp32 merge_bin -o "$output" \
  --flash_size 4MB \
  0x1000 "$bootloader" \
  0x8000 "$partitions" \
  0xe000 "$boot_app0" \
  0x10000 "$firmware" \
  "$spiffs_offset" "$filesystem"

python3 - "$partition_file" "$output" "$flash_size" "$bootloader" "$partitions" "$boot_app0" "$firmware" "$filesystem" <<'PY'
import csv
import os
import sys

partition_file, output, flash_size, bootloader, partitions, boot_app0, firmware, filesystem = sys.argv[1:]
flash_size = int(flash_size)


def decode_partition_table(path):
    entries = []
    with open(path, 'rb') as source:
        while True:
            entry = source.read(32)
            if len(entry) != 32 or entry == b'\xff' * 32:
                break
            # ESP-IDF appends an MD5 checksum record after the partition
            # entries. It is not itself a partition table entry.
            if entry[0:2] == b'\xeb\xeb':
                break
            if entry[0:2] != b'\xaa\x50':
                raise SystemExit(f'ERROR: invalid partition table entry in {path}')
            entries.append({
                'type': entry[2],
                'subtype': entry[3],
                'offset': int.from_bytes(entry[4:8], 'little'),
                'size': int.from_bytes(entry[8:12], 'little'),
                'label': entry[12:28].split(b'\0', 1)[0].decode('ascii'),
            })
    if not entries:
        raise SystemExit(f'ERROR: no partitions found in {path}')
    return entries


def expected_csv_entries(path):
    type_values = {'app': 0, 'data': 1}
    subtype_values = {'nvs': 2, 'ota': 0, 'ota_0': 0x10, 'spiffs': 0x82}
    entries = []
    with open(path, newline='', encoding='utf-8') as source:
        for row in csv.reader(source):
            if not row or row[0].strip().startswith('#'):
                continue
            name, entry_type, subtype, offset, size, *_ = [field.strip() for field in row]
            entries.append({
                'label': name,
                'type': type_values[entry_type],
                'subtype': subtype_values[subtype],
                'offset': int(offset, 0),
                'size': int(size, 0),
            })
    return entries


actual_partitions = decode_partition_table(partitions)
expected_partitions = expected_csv_entries(partition_file)
if actual_partitions != expected_partitions:
    raise SystemExit(f'ERROR: {partitions} does not match {partition_file}: {actual_partitions!r}')

by_label = {entry['label']: entry for entry in actual_partitions}
spiffs = by_label.get('spiffs')
app = by_label.get('app0')
if spiffs is None or spiffs['type'] != 1 or spiffs['subtype'] != 0x82:
    raise SystemExit('ERROR: expected data/spiffs partition label "spiffs" is missing')
if app is None or app['type'] != 0 or app['offset'] != 0x10000:
    raise SystemExit('ERROR: expected app0 partition at 0x10000 is missing')

component_regions = [
    (0x1000, bootloader, 'bootloader'),
    (0x8000, partitions, 'partition table'),
    (0xe000, boot_app0, 'boot metadata'),
    (app['offset'], firmware, 'application'),
    (spiffs['offset'], filesystem, 'LittleFS'),
]
regions = sorted((offset, offset + os.path.getsize(path), name, path)
                 for offset, path, name in component_regions)

if os.path.getsize(output) == 0:
    raise SystemExit('ERROR: merged image is empty')
if os.path.getsize(output) != flash_size:
    raise SystemExit('ERROR: merged image does not cover exactly the configured 4 MiB flash')
if os.path.getsize(output) > flash_size:
    raise SystemExit('ERROR: merged image exceeds the configured 4 MiB flash')
for previous, current in zip(regions, regions[1:]):
    if previous[1] > current[0]:
        raise SystemExit(f'ERROR: overlapping regions: {previous[2]} and {current[2]}')
if regions[-1][1] > flash_size:
    raise SystemExit('ERROR: a flash component exceeds the configured 4 MiB flash')
if os.path.getsize(firmware) > app['size']:
    raise SystemExit('ERROR: application does not fit in app0')
if os.path.getsize(filesystem) != spiffs['size']:
    raise SystemExit('ERROR: LittleFS image size does not match the spiffs partition')

with open(output, 'rb') as merged:
    embedded_partitions = merged.read(0x8000 + os.path.getsize(partitions))[0x8000:]
    with open(partitions, 'rb') as partition_binary:
        expected_partition_binary = partition_binary.read()
    if embedded_partitions != expected_partition_binary:
        raise SystemExit('ERROR: merged image partition table differs from partitions.bin')
    for offset, path, name in component_regions:
        merged.seek(offset)
        with open(path, 'rb') as component:
            expected_component = component.read()
        if merged.read(os.path.getsize(path)) != expected_component:
            raise SystemExit(f'ERROR: merged image {name} bytes differ at 0x{offset:x}')

print(f'Validated {output}: {os.path.getsize(output)} bytes; app0 at 0x{app["offset"]:x}; spiffs at 0x{spiffs["offset"]:x}.')
PY

echo "Wokwi image ready: $output"
