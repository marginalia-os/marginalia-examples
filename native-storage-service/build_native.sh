#!/bin/sh
set -eu

package_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sdk_dir=${MARGINALIA_SDK_DIR:-"$package_dir/../../marginalia-sdk"}
default_cc=/Users/sabraman/.platformio/packages/toolchain-riscv32-esp/bin/riscv32-esp-elf-gcc
cc=${RISCV_CC:-${MARGINALIA_RISCV_CC:-}}
if [ -z "$cc" ]; then
  cc=$(command -v riscv32-esp-elf-gcc || true)
fi
if [ -z "$cc" ]; then
  cc=$default_cc
fi

if ! command -v "$cc" >/dev/null 2>&1 && [ ! -x "$cc" ]; then
  echo "RISC-V compiler not found: $cc" >&2
  echo "Set RISCV_CC to a riscv32-esp-elf-gcc-compatible compiler." >&2
  exit 1
fi

mkdir -p "$package_dir/bin/esp32-c3" "$package_dir/build"

"$cc" \
  -march=rv32imc -mabi=ilp32 \
  -Wall -Wextra -Werror -Os -ffreestanding -fPIC -fvisibility=hidden \
  -fno-builtin -fno-stack-protector -fno-common \
  -nostdlib -nodefaultlibs -nostartfiles \
  -I"$sdk_dir/include" \
  -Wl,--hash-style=sysv \
  -Wl,--no-undefined \
  -Wl,-z,notext \
  -Wl,--entry=marginalia_module_entry_v1 \
  -shared \
  -o "$package_dir/bin/esp32-c3/module.native" \
  "$package_dir/src/module.c"

if command -v sha256sum >/dev/null 2>&1; then
  hash=$(sha256sum "$package_dir/bin/esp32-c3/module.native" | awk '{print $1}')
else
  hash=$(shasum -a 256 "$package_dir/bin/esp32-c3/module.native" | awk '{print $1}')
fi
HASH="$hash" MANIFEST="$package_dir/manifest.json" python3 - <<'PY'
import json
import os
from pathlib import Path

manifest_path = Path(os.environ["MANIFEST"])
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
manifest["artifact"]["sha256"] = os.environ["HASH"]
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
PY

echo "built $package_dir/bin/esp32-c3/module.native"
echo "sha256=$hash"
