[![中文](https://img.shields.io/badge/中文-文档-blue)](ARCHITECTURE.zh.md)

# Architecture

## Runtime flow

```text
Reset_Handler
  -> main()
     -> HAL and clock initialization
     -> MX_GPIO_Init()
     -> MX_SubGHz_Phy_Init()
        -> SystemApp_Init()
        -> SubghzApp_Init()
           -> UART command bridge
           -> AT/transparent state
           -> radio task and callbacks
     -> while (1)
        -> MX_SubGHz_Phy_Process()
           -> UTIL_SEQ_Run()
```

UART receive interrupts deliver bytes through `usart_if.c`. `subg_command.c` bridges the bytes into `lora_transparent_at.c`, which distinguishes AT mode from transparent mode. Radio work is scheduled rather than performed entirely inside the interrupt callback.

## Layers

| Layer | Responsibilities | Main paths |
|---|---|---|
| Application protocol | AT parsing, transparent framing, addresses, configuration and status text | `SubGHz_Phy/App/lora_transparent_at.*`, `subg_command.*` |
| Application orchestration | Task registration, radio state/events and startup messages | `SubGHz_Phy/App/subghz_phy_app.*`, `app_subghz_phy.*` |
| Platform interfaces | UART, GPIO, RTC/timer, low power and board RF switch | `Core/*`, `SubGHz_Phy/Target/*` |
| Radio middleware | Generic radio API and STM32WL SubGHz driver | `Middlewares/Third_Party/SubGHz_Phy/*` |
| Device support | CMSIS, HAL and NUCLEO-WL55JC BSP | `Drivers/*` |

## Data paths

### AT mode

`LPUART1 RX interrupt` → UART callback → command bridge → AT parser → configuration/state change → response over LPUART1.

### Transparent transmit

UART bytes → idle-delimited packet → address header and checksum → scheduled radio send → `TXDONE`, `TXTIMEOUT` or `BUSY` response.

### Transparent receive

Radio callback → frame/header/checksum/address checks → RSSI/SNR and payload output over LPUART1, or an error response.

## Persistent and volatile state

The implementation stores radio configuration in internal Flash at its configured address. Local and target addresses remain volatile. Changes to the Flash layout, linker memory regions or configuration structure require compatibility review before release.

## Hardware assumptions

The checked project targets `STM32WL55JCIx` / Cortex-M4 and uses NUCLEO-WL55JC support files. The actual Ra-09 module wiring and RF matching are not inferred beyond what the committed source and project files express.
