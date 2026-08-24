[![English](https://img.shields.io/badge/English-Docs-green)](README.md)

# UART 转 LoRa 应用

该 STM32WL55 应用提供兼容 Ra-08 的串口 AT 接口和 LoRa 透传功能。上电后进入 AT 模式；通过 `AT+CTX` 配置地址和射频参数后，串口原始数据会被封装并通过 LoRa 发送。

## 接口

- 串口：LPUART1，9600 波特率，8 数据位、无校验、1 停止位。
- 引脚：PA2 TX、PA3 RX。
- AT 行结束符：CR+LF（`\r\n`）。
- 透传用户载荷最大 247 字节。

## 推荐配置流程

1. 设置本机地址：`AT+CADDR=<地址>`。
2. 设置对端地址：`AT+CTXADDR=<地址>`。
3. 配置 LoRa 并进入透传：`AT+CTX=<freq>,<dr>,<bw>,<cr>,<power>,<iq>`。
4. 直接发送业务数据；串口空闲约 15 ms 后自动结束一帧。
5. 仅发送三个加号 `+++` 返回 AT 模式。

双机通信时，两端射频参数必须一致；A 的本机地址应等于 B 的目标地址，反之亦然。

```text
AT+CADDR=10
AT+CTXADDR=20
AT+CTX=470625000,3,0,1,22,0
hello
```

## 指令

| 指令 | 功能 |
|---|---|
| `AT` | 检查串口指令链路 |
| `AT+CADDR=<addr>` | 设置 16 位本机地址 |
| `AT+CTXADDR=<addr>` | 设置 16 位对端地址 |
| `AT+CTX=<freq>,<dr>,<bw>,<cr>,<pwr>,<iq>` | 配置 LoRa 并进入透传 |
| `AT+CTXCW=<freq>,<pwr>` | 连续波发射，仅用于射频测试 |
| `AT+CSLEEP=<mode>` | 进入当前实现的低功耗流程 |
| `AT+CSTDBY=<mode>` | 进入当前实现的待机/低功耗流程 |
| `+++` | 退出透传模式 |

指令格式不正确时返回 `+CMD ERROR:1`。

## `AT+CTX` 参数

| 参数 | 合法值 | 默认值/越界修正 |
|---|---|---|
| `freq` | 100,000,000–1,000,000,000 Hz | 470,625,000 Hz |
| `dr` | 0–7；扩频因子为 `12 - dr` | 3（SF9） |
| `bw` | 0=125 kHz，1=250 kHz，2=500 kHz | 0 |
| `cr` | 1=4/5，2=4/6，3=4/7，4=4/8 | 1 |
| `pwr` | 0–22 dBm | 22 dBm |
| `iq` | 0=不反转，1=反转 | 0 |

射频参数写入应用指定的 Flash 地址并在下次启动时恢复；当前实现不会持久化本机地址和对端地址。

## 透传帧与回显

无线帧由 `0xAA`、2 字节本机地址、2 字节对端地址、载荷和校验和组成。常见回显如下：

| 回显 | 含义 |
|---|---|
| `AT_MODE` | AT 指令模式 |
| `LORA_TRANSPARENT_MODE` | 已进入透传模式 |
| `TXDONE` / `TXTIMEOUT` | 发送完成 / 超时 |
| `RXDONE` / `RXTIMEOUT` / `RXERROR` | 接收结果 |
| `BUSY` | 射频忙，本次串口数据未发送 |

## 构建

GNU Arm Embedded 构建：

```bash
make -f GNUmakefile -j2
```

仓库也提供 STM32CubeIDE 和 Keil 工程。使用生成固件前请阅读仓库级[验证证据](../../../../../docs/VALIDATION.zh.md)。

## 安全与验证边界

请根据所在地区法规选择允许的频率和功率。连续波模式仅用于受控射频测试。自动化检查会编译并链接固件，但不会烧录模块，也不验证串口时序、射频输出、法规符合性、距离、休眠电流或双机互通。
