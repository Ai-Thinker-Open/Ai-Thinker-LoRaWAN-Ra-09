[![English](https://img.shields.io/badge/English-Docs-green)](ARCHITECTURE.md)

# 架构说明

## 运行流程

```text
Reset_Handler
  -> main()
     -> HAL 与时钟初始化
     -> MX_GPIO_Init()
     -> MX_SubGHz_Phy_Init()
        -> SystemApp_Init()
        -> SubghzApp_Init()
           -> 串口指令桥接
           -> AT/透传状态
           -> 射频任务与回调
     -> while (1)
        -> MX_SubGHz_Phy_Process()
           -> UTIL_SEQ_Run()
```

串口接收中断通过 `usart_if.c` 分发字节，`subg_command.c` 再把字节交给 `lora_transparent_at.c`，由它区分 AT 模式和透传模式。射频工作通过任务调度执行，不会全部堆在中断回调内。

## 分层

| 层 | 职责 | 主要路径 |
|---|---|---|
| 应用协议 | AT 解析、透传组帧、地址、配置和状态文本 | `SubGHz_Phy/App/lora_transparent_at.*`、`subg_command.*` |
| 应用编排 | 任务注册、射频状态/事件和启动信息 | `SubGHz_Phy/App/subghz_phy_app.*`、`app_subghz_phy.*` |
| 平台接口 | 串口、GPIO、RTC/定时器、低功耗和板级射频开关 | `Core/*`、`SubGHz_Phy/Target/*` |
| 射频中间件 | 通用 Radio API 与 STM32WL SubGHz 驱动 | `Middlewares/Third_Party/SubGHz_Phy/*` |
| 器件支持 | CMSIS、HAL 和 NUCLEO-WL55JC BSP | `Drivers/*` |

## 数据路径

### AT 模式

`LPUART1 接收中断` → 串口回调 → 指令桥接 → AT 解析器 → 配置/状态变化 → LPUART1 回显。

### 透传发送

串口字节 → 按空闲时间结束一包 → 添加地址头和校验和 → 调度射频发送 → 返回 `TXDONE`、`TXTIMEOUT` 或 `BUSY`。

### 透传接收

射频回调 → 帧头/校验和/地址检查 → 通过 LPUART1 输出 RSSI、SNR 和载荷，或返回错误。

## 持久与易失状态

当前实现在内部 Flash 的指定地址保存射频配置；本机地址和目标地址是易失状态。修改 Flash 布局、链接内存区域或配置结构前，必须进行兼容性审查。

## 硬件假设

已检查工程的目标为 `STM32WL55JCIx` / Cortex-M4，并使用 NUCLEO-WL55JC 支持文件。除已提交源码和工程文件明确表达的内容外，本说明不推断 Ra-09 模块的实际接线和射频匹配。
