#!/bin/sh
# 模块 QEMU 加载回归：用本镜像自带的内核加载指定 .ko，三态判定。
# 用法: /test/run.sh <module.ko> [kmi]     （kmi 省略时自动取镜像内唯一的 kdir）
set -e
KO="$1"
[ -n "$KO" ] && [ -f "$KO" ] || { echo "usage: run.sh <module.ko> [kmi]"; exit 2; }

if [ -n "$2" ]; then
    KMI="$2"
else
    KMI=$(basename "$(ls -d /opt/ddk/kdir/* | head -1)")
fi
KERNEL="/opt/ddk/kdir/$KMI/arch/arm64/boot/Image"
[ -f "$KERNEL" ] || { echo "kernel Image not found: $KERNEL (kmi=$KMI)"; exit 2; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
python3 /test/build-initramfs.py "$KO" "$WORK/rootfs.cpio.gz" /test/init

echo "== QEMU boot: $KMI, module: $KO =="
timeout 300 qemu-system-aarch64 -M virt -cpu max -m 4G -smp 4 -nographic \
    -kernel "$KERNEL" -initrd "$WORK/rootfs.cpio.gz" \
    -append "console=ttyAMA0 earlycon=pl011,mmio32,0x09000000 rdinit=/init panic=-1" \
    -no-reboot -nic none > "$WORK/console.log" 2>&1 || true

cp "$WORK/console.log" ./console.log 2>/dev/null || true
tail -40 "$WORK/console.log"

if grep -q "MODULE LOADED OK" "$WORK/console.log"; then
    echo "== PASS: MODULE LOADED OK =="
    exit 0
elif grep -qE "Internal error|Kernel panic" "$WORK/console.log"; then
    echo "== FAIL: 内核崩溃（CFI/Oops），完整日志见 console.log =="
    exit 1
else
    echo "== FAIL: 模块被加载器拒绝，errno/vermagic 见 console.log =="
    exit 1
fi
