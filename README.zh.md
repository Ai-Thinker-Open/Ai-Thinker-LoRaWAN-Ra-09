[![English](https://img.shields.io/badge/English-README-green)](README.md)

# 安信可 LoRaWAN Ra-09 SDK

本仓库是安信可 Ra-09 SDK 使用的 STM32CubeWL 固件包。代码基于 STMicroelectronics STM32CubeWL，并包含面向 STM32WL55 Cortex-M4 的 UART 转 LoRa 应用。

## 从这里开始

与产品直接相关的工程位于：

```text
Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa/
```

- [UART 转 LoRa 使用方法与 AT 指令](Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa/README.zh.md)
- [代码入口](docs/CODE_ENTRY.zh.md)
- [架构说明](docs/ARCHITECTURE.zh.md)
- [构建与验证证据](docs/VALIDATION.zh.md)
- [软件包许可条款](Package_license.md)

## 支持的工程形式

| 工具链 | 工程/构建入口 | 状态 |
|---|---|---|
| GNU Arm Embedded | `UART_To_LoRa` 中的 `GNUmakefile` | 已使用 GCC 10.3.1 完成可重复构建 |
| STM32CubeIDE | `UART_To_LoRa/STM32CubeIDE/.project` | 已修复工程引用；本次未在 IDE 内执行 |
| Keil MDK-ARM | `UART_To_LoRa/MDK-ARM/*.uvprojx` | 工程存在；本次未执行 |

## GNU 构建

安装 `make` 和 `arm-none-eabi-gcc` 后执行：

```bash
cd Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa
make -f GNUmakefile -j2
```

构建产物 ELF、HEX 和 BIN 位于 `build/`。执行 `make -f GNUmakefile clean` 可清理产物。

## 软件包范围

仓库其余部分是 STM32CubeWL 平台软件包，包括 CMSIS、STM32 HAL/LL 驱动、BSP、中间件、工具、示例和演示。修改厂商代码前请先阅读 ST 的组件文档。

## 验证边界

已记录的构建未烧录硬件。串口 AT 行为、射频性能、通信距离、休眠唤醒、Flash 参数保持以及双机互通仍需 Ra-09 实物验证，本仓库不把这些项目标记为已验证。

## 许可证

仓库内不同组件适用不同许可证。软件包级条款及组件许可证矩阵见 [Package_license.md](Package_license.md)。分发和修改时应保留各组件原有版权与许可声明。
