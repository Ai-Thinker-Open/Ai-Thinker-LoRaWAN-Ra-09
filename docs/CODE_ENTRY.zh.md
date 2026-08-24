[![English](https://img.shields.io/badge/English-Docs-green)](CODE_ENTRY.md)

# 代码入口

## 产品应用

Ra-09 相关实现主要位于 `Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa`。仓库其余部分主要是 STM32CubeWL 平台软件包和示例。

| 关注点 | 入口文件或符号 | 仓库内证据 |
|---|---|---|
| 复位与中断向量 | `Drivers/CMSIS/Device/ST/STM32WLxx/Source/Templates/gcc/startup_stm32wl55xx_cm4.s` | 定义 Cortex-M4 向量表并进入 C 运行库 |
| C 入口 | `Core/Src/main.c` → `main()` | 初始化 HAL、时钟、GPIO 和 SubGHz 应用，并循环调用 `MX_SubGHz_Phy_Process()` |
| 应用初始化 | `SubGHz_Phy/App/app_subghz_phy.c` → `MX_SubGHz_Phy_Init()` | 调用 `SystemApp_Init()` 和 `SubghzApp_Init()` |
| 协作式处理 | `SubGHz_Phy/App/app_subghz_phy.c` → `MX_SubGHz_Phy_Process()` | 运行 `UTIL_SEQ_Run()` |
| 指令/任务注册 | `SubGHz_Phy/App/subghz_phy_app.c` → `SubghzApp_Init()` | 注册射频处理任务并启动指令处理 |
| 串口接收桥接 | `SubGHz_Phy/App/subg_command.c` | 把接收到的串口字节交给透传/AT 解析器 |
| AT 与透传模式 | `SubGHz_Phy/App/lora_transparent_at.c` | 解析指令、管理地址和配置，并连接串口与射频处理 |
| 射频事件/配置层 | `SubGHz_Phy/App/subghz_phy_app.c` 和 `SubGHz_Phy/Target/radio_board_if.c` | 将应用回调连接到 STM32 SubGHz 射频驱动和板级开关控制 |
| 串口 HAL | `Core/Src/usart.c` 和 `Core/Src/usart_if.c` | 配置 LPUART1 并分发收发回调 |

## 构建入口

- GNU Arm Embedded：在 `UART_To_LoRa` 目录执行 `make -f GNUmakefile`。
- STM32CubeIDE：把 `UART_To_LoRa/STM32CubeIDE` 作为已有工程导入。
- Keil MDK-ARM：打开 `UART_To_LoRa/MDK-ARM/SubGHz_Phy_PingPong.uvprojx`；为兼容已有用户保留历史工程名。

GNU 构建的源文件清单来自 Keil 工程，并包含 AT/透传源文件；STM32CubeIDE 的链接资源清单已与其对齐。

## 产品路径之外

`Drivers`、`Middlewares`、`Utilities` 以及大部分其他工程属于 STM32CubeWL 上游组件。除非产品修改确实需要，否则应把它们视为厂商依赖。
