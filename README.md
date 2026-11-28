# Tensor-mainline: Pixel 9 Pro on mainline Linux

Mainline Linux kernel bring-up for the **Google Pixel 9 Pro ("caiman")**, built on the **Tensor G4 ("zumapro")** SoC.

This is a hobby port on top of upstream Linux (currently based on **v7.4-rc3**). The goal is a phone that boots a normal Linux userland with display, touch, storage, USB and WiFi, using upstream drivers where they exist and small, upstreamable drivers where they don't. If it goes well, parts of it may be sent upstream.

> **Status: experimental.** Expect rough edges, debug knobs and things that change from week to week. Flashing custom kernels can leave your phone unbootable until you reflash a stock image. You do this at your own risk.

## Branches

| Branch | What it is |
|---|---|
| `Sovereign` | The main development branch. Everything below refers to this branch. |

## Hardware status

| Area | Status | Notes |
|---|---|---|
| Boot, CPUs, UART console | ✅ Working | Serial console at 921600 baud |
| Clocks (CMU) | ⏱ Working | TOP, MISC, PERIC0, HSI0, HSI2 and G3D controllers |
| Pin control, GPIO keys | 📍 Working | Power and volume keys |
| PMICs (S2MPG14/S2MPG15) | ⚡️ Working | Probed over ACPM. Regulators and the RTC work |
| CPU frequency scaling | 🎛 Working | ACPM DVFS |
| Thermal | 🌡 Working | TMU zones for CPU and GPU |
| UFS storage | 💾 Working | |
| Display | 📱 Working | zumapro_decon |
| GPU (Mali-G715) | 📱 Working | Panthor|
| Touchscreen (Synaptics S3908) | 👇 Working | TouchComm SPI driver |
| USB | 📲 Working | Peripheral mode only, with USB gadget networking |
| Battery fuel gauge (MAX77779) | 🔋 Working | Read only |
| PCIe | 🚄 Working | A new Tensor G4 root complex driver. Runtime suspend and link-down recovery are not done yet |
| WiFi (BCM4390) | 🛜 Working | Uses Broadcom's out-of-tree `bcmdhd` driver. Supports monitor mode. |
| Modem, audio, cameras, charging, Bluetooth, NFC | ❌ Not started | |

## Building

You need an aarch64 cross toolchain, either GCC or LLVM.

```sh
git clone -b Sovereign https://github.com/WoodenWillis/Tensor-mainline.git
cd Tensor-mainline

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- caiman_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)" Image dtbs
```

This produces:

- `arch/arm64/boot/Image`
- `arch/arm64/boot/dts/exynos/google/caiman.dtb`

The defconfig embeds an initramfs. Point `CONFIG_INITRAMFS_SOURCE` at your own initramfs directory or list before you build.

### WiFi

There is no mainline driver for the BCM4390, so the port carries Broadcom's FullMAC DHD driver as an out-of-tree module in `tools/caiman/bcmdhd/`. Its history is in two commits:

1. A pristine import of `google-modules/wlan/bcm4398` from the caiman vendor tree (DHD 103.101.26.24).
2. The mainline port on top. This covers cfg80211 API drift, no-op hooks in place of the vendor PCIe RC, MSI on by default, the BCM4390 b1 chip tables, and a fix for duplicate cipher suites.

Keeping the import untouched means the port reads as a plain diff against what Google ships.

Build it against the same kernel tree that runs on the phone:

```sh
make -C tools/caiman/bcmdhd KERNEL_SRC="$PWD" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

The kernel is built without `MODVERSIONS`, so rebuild the module after any kernel config change. The driver also needs the BCM4390 firmware and NVRAM files from the stock Pixel image. They aren't included here.

## Layout

New code :

- Device tree: `arch/arm64/boot/dts/exynos/google/` (`zumapro.dtsi`, `zumapro-pinctrl.dtsi`, `zumapro-tmu.dtsi`, `caiman.dts`)
- Defconfig: `arch/arm64/configs/caiman_defconfig`
- Clocks: `drivers/clk/samsung/clk-zumapro.c`
- Display: `drivers/gpu/drm/sysfb/zumapro_decon.c`
- Touchscreen: `drivers/input/touchscreen/synaptics_tcm.c`
- PCIe: `drivers/pci/controller/dwc/pcie-zumapro.c`
- UFS PHY: `drivers/phy/samsung/phy-zumapro-ufs.c`
- GPU DVFS domain: `drivers/pmdomain/samsung/zumapro-gpu-dvfs.c`
- Fuel gauge: `drivers/power/supply/max77779_fg.c`
- Thermal: `drivers/thermal/samsung/acpm-tmu.c`
- WiFi (out-of-tree module): `tools/caiman/bcmdhd/`

Device tree bindings for all of the above are in `Documentation/devicetree/bindings/`.

## Credits

- The upstream Linux community, and especially the people behind the **Google Tensor GS101 (Pixel 6)** mainline work. A lot of this port builds directly on that.
- **Tudor Ambarus (Linaro)** for the GS101 ACPM TMU thermal driver this port carries and extends.
- **Broadcom and Google** for the `bcmdhd` WiFi driver, which is GPL-2.0 licensed and carried here with its original notices.
- Samsung and Google, whose GPL-licensed downstream sources helped with register layouts. Their copyright notices are kept in the files that derive from them.

## License

Same as the Linux kernel: **GPL-2.0-only**, with the exceptions described in [`COPYING`](COPYING) and the [`LICENSES/`](LICENSES) directory. Individual files carry SPDX identifiers.
