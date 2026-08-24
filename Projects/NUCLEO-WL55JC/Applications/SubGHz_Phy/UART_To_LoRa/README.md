[![中文](https://img.shields.io/badge/中文-文档-blue)](README.zh.md)

# UART to LoRa application

This STM32WL55 application provides an Ra-08-compatible UART AT interface and transparent LoRa transport. It starts in AT mode. After configuring addresses and radio parameters with `AT+CTX`, raw UART data is framed and sent over LoRa.

## Interface

- UART: LPUART1, 9600 baud, 8 data bits, no parity, 1 stop bit.
- Pins: PA2 TX, PA3 RX.
- AT line ending: CR+LF (`\r\n`).
- Maximum transparent user payload: 247 bytes.

## Recommended setup

1. Set the local address: `AT+CADDR=<address>`.
2. Set the peer address: `AT+CTXADDR=<address>`.
3. Configure LoRa and enter transparent mode:
   `AT+CTX=<freq>,<dr>,<bw>,<cr>,<power>,<iq>`.
4. Send raw application bytes. About 15 ms of UART idle closes a packet.
5. Send exactly `+++` to return to AT mode.

For two devices, use identical radio parameters. Device A's local address must equal device B's target address, and vice versa.

```text
AT+CADDR=10
AT+CTXADDR=20
AT+CTX=470625000,3,0,1,22,0
hello
```

## Commands

| Command | Purpose |
|---|---|
| `AT` | Check the UART command link |
| `AT+CADDR=<addr>` | Set the 16-bit local address |
| `AT+CTXADDR=<addr>` | Set the 16-bit peer address |
| `AT+CTX=<freq>,<dr>,<bw>,<cr>,<pwr>,<iq>` | Configure LoRa and enter transparent mode |
| `AT+CTXCW=<freq>,<pwr>` | Start continuous-wave transmission for RF testing |
| `AT+CSLEEP=<mode>` | Enter the implemented low-power flow |
| `AT+CSTDBY=<mode>` | Enter the implemented standby/low-power flow |
| `+++` | Leave transparent mode |

An invalid command returns `+CMD ERROR:1`.

## `AT+CTX` parameters

| Parameter | Accepted value | Default/correction |
|---|---|---|
| `freq` | 100,000,000–1,000,000,000 Hz | 470,625,000 Hz |
| `dr` | 0–7; spreading factor is `12 - dr` | 3 (SF9) |
| `bw` | 0=125 kHz, 1=250 kHz, 2=500 kHz | 0 |
| `cr` | 1=4/5, 2=4/6, 3=4/7, 4=4/8 | 1 |
| `pwr` | 0–22 dBm | 22 dBm |
| `iq` | 0=normal, 1=inverted | 0 |

Radio parameters are stored at the application-configured Flash address and restored at startup. Local and peer addresses are not persisted by the current implementation.

## Transparent framing and responses

The transmitted radio frame contains `0xAA`, a two-byte local address, a two-byte peer address, payload and checksum. Common responses are:

| Response | Meaning |
|---|---|
| `AT_MODE` | Command mode |
| `LORA_TRANSPARENT_MODE` | Transparent mode entered |
| `TXDONE` / `TXTIMEOUT` | Transmission completed / timed out |
| `RXDONE` / `RXTIMEOUT` / `RXERROR` | Receive result |
| `BUSY` | Radio busy; the submitted UART packet was not sent |

## Build

GNU Arm Embedded build:

```bash
make -f GNUmakefile -j2
```

STM32CubeIDE and Keil project files are also provided. See the repository-level [validation evidence](../../../../../docs/VALIDATION.md) before using generated firmware.

## Safety and validation boundary

Select a frequency and power permitted in your region. Continuous-wave mode is intended for controlled RF testing. The automated checks compile and link the firmware but do not flash a module or verify UART timing, RF output, regulatory compliance, range, sleep current or interoperability.
