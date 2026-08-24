[![中文](https://img.shields.io/badge/中文-README-blue)](README.zh.md)

# Ai-Thinker LoRaWAN Ra-09 SDK

This repository contains the STM32CubeWL firmware package used by the Ai-Thinker Ra-09 SDK. The repository is based on STMicroelectronics STM32CubeWL and includes an Ai-Thinker UART-to-LoRa application for the STM32WL55 Cortex-M4 target.

## Start here

The product-specific application is located at:

```text
Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa/
```

- [UART-to-LoRa usage and AT commands](Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa/README.md)
- [Code entry points](docs/CODE_ENTRY.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Build and validation evidence](docs/VALIDATION.md)
- [Package license terms](Package_license.md)

## Supported project formats

| Toolchain | Project/build entry | Status |
|---|---|---|
| GNU Arm Embedded | `GNUmakefile` in `UART_To_LoRa` | Reproducibly built with GCC 10.3.1 |
| STM32CubeIDE | `UART_To_LoRa/STM32CubeIDE/.project` | Project references repaired; not exercised with the IDE in this audit |
| Keil MDK-ARM | `UART_To_LoRa/MDK-ARM/*.uvprojx` | Present; not exercised in this audit |

## GNU build

Install `make` and `arm-none-eabi-gcc`, then run:

```bash
cd Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa
make -f GNUmakefile -j2
```

The build creates ELF, HEX and BIN files under `build/`. Run `make -f GNUmakefile clean` to remove them.

## Package scope

The rest of the tree is the STM32CubeWL platform package: CMSIS, STM32 HAL/LL drivers, BSPs, middleware, utilities, examples and demonstrations. Refer to ST's component documentation before changing vendor code.

## Validation boundary

The documented build was performed without flashing hardware. Serial AT behavior, RF performance, range, sleep/wakeup, flash persistence and interoperability require Ra-09 hardware tests and are not claimed as verified.

## License

This repository contains components under different licenses. The package-level terms and component license matrix are in [Package_license.md](Package_license.md). Retain the copyright and license notices of each component.
