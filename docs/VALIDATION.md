[![中文](https://img.shields.io/badge/中文-文档-blue)](VALIDATION.zh.md)

# Build and validation evidence

## Verified on 2026-08-24

| Check | Result |
|---|---|
| Repository structure and required product entry files | Pass |
| English/Chinese documentation pairs and language badges | Pass |
| STM32CubeIDE XML syntax and required linked resources | Pass |
| GNU clean compile and link | Pass |
| Repeated clean build | Pass; binary hashes are identical |
| Hardware flashing and runtime | Not performed |

## Build environment and command

- Ubuntu 22.04 under WSL2
- GNU Make 4.3
- `arm-none-eabi-gcc` 10.3.1 (20210621 release)

```bash
cd Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa
make -f GNUmakefile clean
make -f GNUmakefile -j2
```

The build compiles the application, STM32WL HAL/BSP, SubGHz middleware, utilities and the GCC Cortex-M4 startup file, then links with the STM32WL55JCIX 256 KiB Flash / 64 KiB RAM linker layout already present in the package.

## Observed output

```text
text     data     bss      dec      hex
42280    168      5608     48056    bbb8
```

- BIN size: 42,456 bytes
- BIN SHA-256 from clean build 1: `F7874A01ABF288E951EBA30EA5F996FC801780F75907CFE630BB86D2EA610A6F`
- BIN SHA-256 from clean build 2: `F7874A01ABF288E951EBA30EA5F996FC801780F75907CFE630BB86D2EA610A6F`
- Compiler errors: 0
- Implicit-function-declaration diagnostics: 0
- Warning lines: 55, primarily unused parameters in existing ST middleware/HAL callback interfaces plus one existing fall-through diagnostic

Identical hashes demonstrate deterministic output in the recorded environment; they do not prove correct behavior on hardware.

## Repository fixes covered by automation

- The product's AT and transparent-mode C sources are present in both the GNU source list and STM32CubeIDE linked resources.
- The STM32CubeIDE project resolves a GCC startup file and linker script available in this repository.
- Windows `Zone.Identifier` alternate-stream artifacts are absent from the tracked tree.
- Documentation pairs, language-switch badges and selected local links are checked.

Run the repository checks with:

```bash
python3 tools/validate_repository.py
```

## Required hardware validation before release

1. Flash the generated image to the intended Ra-09 hardware and capture the programmer result.
2. Verify 9600 8N1 AT parsing, CR+LF handling and invalid-command responses.
3. Verify two-device address filtering, payload boundaries, `+++`, busy/timeout recovery and continuous reception.
4. Power-cycle after changing radio parameters and confirm the intended persistence behavior.
5. Measure frequency, output power, current consumption and RF performance using suitable instruments and region-appropriate settings.

Until these tests are recorded, the firmware is build-verified, not hardware-qualified.
