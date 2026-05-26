# UART_To_LoRa — 串口转 LoRa 透传例程

## 功能概述

UART_To_LoRa 是一个简易的 UART → LoRa 无线收发例程：

1. **USART2** (PA2=TX, PA3=RX) 配置为 **9600 8N1**，开启接收中断
2. 中断收到的数据写入 **1KB RingBuffer**（ISR 安全）
3. **TIM2** 每 **10ms** 扫描 RingBuffer
4. 若有数据，立即通过 **LoRa** 无线发出

初始化完成后，串口输出 `UART_To_LoRa_Init_DONE`。

---

## LoRa 射频参数

| 参数 | 值 |
|------|-----|
| 频率 | **470.5 MHz** |
| 带宽 | **125 kHz** |
| 编码率 | **4/5** |
| 扩频因子 | **SF7** |
| 发射功率 | **22 dBm** |
| 前导码 | 8 symbols |

---

## 硬件说明

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA2 | USART2_TX | 连接 ST-LINK VCP (USB 串口) |
| PA3 | USART2_RX | 连接 ST-LINK VCP (USB 串口) |

> 使用 NUCLEO-WL55JC 板载 ST-LINK 虚拟串口，通过 USB 直接通信。

---

## 编译

```bash
cd /path/to/STM32CubeWL
python3 cmake/tools/patch_wle5_dma_privilege.py
python3 cmake/tools/generate_wle5_cmake.py
./build.sh WLE5 UART_To_LoRa
```

产物路径：
```
build_result/WLE5/NUCLEO_WL55JC_Applications_SubGHz_Phy_UART_To_LoRa/
├── NUCLEO_WL55JC_Applications_SubGHz_Phy_UART_To_LoRa.bin
├── NUCLEO_WL55JC_Applications_SubGHz_Phy_UART_To_LoRa.hex
├── NUCLEO_WL55JC_Applications_SubGHz_Phy_UART_To_LoRa.elf
└── NUCLEO_WL55JC_Applications_SubGHz_Phy_UART_To_LoRa.map
```

---

## 使用步骤

### 1. 烧录与连接

将 `.bin` 或 `.hex` 烧录到 NUCLEO-WL55JC（拖入 ST-LINK 虚拟磁盘，或用 STM32CubeProgrammer）。

### 2. 打开串口

- 波特率：**9600**
- 数据位：8
- 停止位：1
- 校验：无
- 流控：无

### 3. 验证初始化

上电后串口输出：

```
UART_To_LoRa_Init_DONE
```

### 4. 发送数据

直接向串口发送任意数据（最大 254 字节/包），数据会自动通过 LoRa 发出。

> 需要另一块 NUCLEO-WL55JC 运行相同固件（LoRa 参数必须一致）作为接收端。

---

## 数据流

```
串口终端 → USB → ST-LINK VCP
                    ↓ PA3
              USART2 RX ISR (每字节)
                    ↓
              1KB RingBuffer
                    ↓
           TIM2 10ms 轮询扫描
                    ↓ (有数据)
            Radio.Send()  LoRa 470.5MHz
                    ↓
              无线 → 对端设备
```

---

## 时序分析

| 指标 | 值 |
|------|-----|
| UART 速率 | 9600 baud ≈ 960 字节/秒 |
| 10ms 内到达数据 | ≤ 10 字节 |
| RingBuffer 容量 | 1024 字节 (可缓冲 ~1 秒) |
| LoRa 空中时间 (254字节) | ~56ms @ SF7/BW125k |
| 单包最大 | 254 字节 |

---

## 工程结构

```
UART_To_LoRa/
├── Core/Src/
│   ├── main.c              ← 入口: 时钟/外设初始化, 主循环
│   ├── usart.c             ← USART2 9600 8N1 配置
│   ├── stm32wlxx_it.c      ← 中断处理 (USART2/TIM2/SUBGHZ)
│   └── ...                 ← 桩文件 (gpio/dma/rtc/...)
├── SubGHz_Phy/App/
│   ├── subghz_phy_app.h    ← 配置宏 (LoRa/Timer/RingBuffer)
│   └── subghz_phy_app.c    ← 核心逻辑 (RingBuffer/TIM2/LoRa TX)
└── SubGHz_Phy/Target/      ← 射频板级配置
```

---

## 限制

- 本例程为单向传输（UART 输入 → LoRa 发送），LoRa 接收仅进入 RX 等待不处理
- 单包最大 254 字节
- 无流控：UART 持续高速发送可能导致 LoRa 拥塞丢包
- 9600 baud 下 10ms 轮询足够安全，提高波特率需减小轮询周期
