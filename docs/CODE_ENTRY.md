[![中文](https://img.shields.io/badge/中文-文档-blue)](CODE_ENTRY.zh.md)

# Code entry points

## Product application

The Ra-09-specific work is concentrated under `Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa`. The remaining repository is the STM32CubeWL platform package and examples.

| Concern | Entry file or symbol | Evidence in the repository |
|---|---|---|
| Reset and vector table | `Drivers/CMSIS/Device/ST/STM32WLxx/Source/Templates/gcc/startup_stm32wl55xx_cm4.s` | Defines the Cortex-M4 vector table and calls the C runtime |
| C entry point | `Core/Src/main.c` → `main()` | Initializes HAL, clock, GPIO and SubGHz application; repeatedly calls `MX_SubGHz_Phy_Process()` |
| Application initialization | `SubGHz_Phy/App/app_subghz_phy.c` → `MX_SubGHz_Phy_Init()` | Calls `SystemApp_Init()` and `SubghzApp_Init()` |
| Cooperative processing | `SubGHz_Phy/App/app_subghz_phy.c` → `MX_SubGHz_Phy_Process()` | Runs `UTIL_SEQ_Run()` |
| Command/task registration | `SubGHz_Phy/App/subghz_phy_app.c` → `SubghzApp_Init()` | Registers the radio process and starts command handling |
| UART receive bridge | `SubGHz_Phy/App/subg_command.c` | Passes received UART bytes to the transparent/AT parser |
| AT and transparent mode | `SubGHz_Phy/App/lora_transparent_at.c` | Parses commands, manages addresses/configuration and moves data between UART and radio processing |
| Radio event/configuration layer | `SubGHz_Phy/App/subghz_phy_app.c` and `SubGHz_Phy/Target/radio_board_if.c` | Connects application callbacks to the STM32 SubGHz radio driver and board switch control |
| UART HAL | `Core/Src/usart.c` and `Core/Src/usart_if.c` | Configures LPUART1 and delivers transmit/receive callbacks |

## Build entry points

- GNU Arm Embedded: run `make -f GNUmakefile` inside the `UART_To_LoRa` directory.
- STM32CubeIDE: import `UART_To_LoRa/STM32CubeIDE` as an existing project.
- Keil MDK-ARM: open `UART_To_LoRa/MDK-ARM/SubGHz_Phy_PingPong.uvprojx` (the historical project name is retained for compatibility).

The GNU build source list is derived from the Keil project and includes the AT/transparent sources. The STM32CubeIDE linked-resource list was aligned with those sources.

## Areas outside the product path

`Drivers`, `Middlewares`, `Utilities` and most other projects are upstream STM32CubeWL components. Treat them as vendor dependencies unless a product change explicitly requires modifying them.
