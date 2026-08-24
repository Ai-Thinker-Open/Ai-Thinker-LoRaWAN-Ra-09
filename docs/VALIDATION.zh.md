[![English](https://img.shields.io/badge/English-Docs-green)](VALIDATION.md)

# 构建与验证证据

## 2026-08-24 已验证项目

| 检查 | 结果 |
|---|---|
| 仓库结构和必要产品入口文件 | 通过 |
| 中英文文档配对及语言切换徽章 | 通过 |
| STM32CubeIDE XML 语法和必要链接资源 | 通过 |
| GNU 干净编译与链接 | 通过 |
| 重复干净构建 | 通过；二进制哈希一致 |
| 硬件烧录和运行 | 未执行 |

## 构建环境和命令

- WSL2 中的 Ubuntu 22.04
- GNU Make 4.3
- `arm-none-eabi-gcc` 10.3.1（20210621 release）

```bash
cd Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/UART_To_LoRa
make -f GNUmakefile clean
make -f GNUmakefile -j2
```

该构建会编译应用、STM32WL HAL/BSP、SubGHz 中间件、工具和 GCC Cortex-M4 启动文件，然后使用软件包内已有的 STM32WL55JCIX 256 KiB Flash / 64 KiB RAM 链接布局完成链接。

## 实测输出

```text
text     data     bss      dec      hex
42280    168      5608     48056    bbb8
```

- BIN 大小：42,456 字节
- 第 1 次干净构建 BIN SHA-256：`F7874A01ABF288E951EBA30EA5F996FC801780F75907CFE630BB86D2EA610A6F`
- 第 2 次干净构建 BIN SHA-256：`F7874A01ABF288E951EBA30EA5F996FC801780F75907CFE630BB86D2EA610A6F`
- 编译错误：0
- 隐式函数声明诊断：0
- 警告行：55；主要来自既有 ST 中间件/HAL 回调接口的未使用参数，另有一处既有 fall-through 诊断

哈希一致说明在记录环境中产物可重复，但不能证明硬件行为正确。

## 自动化覆盖的仓库修复

- 产品 AT 与透传 C 源文件同时存在于 GNU 源文件清单和 STM32CubeIDE 链接资源中。
- STM32CubeIDE 工程能引用仓库内已有的 GCC 启动文件和链接脚本。
- 跟踪树中不存在 Windows `Zone.Identifier` 附加流垃圾条目。
- 检查文档配对、语言切换徽章和选定的本地链接。

执行仓库检查：

```bash
python3 tools/validate_repository.py
```

## 发布前必须补做的硬件验证

1. 把生成固件烧录到目标 Ra-09 硬件，并保留烧录结果。
2. 验证 9600 8N1 AT 解析、CR+LF 处理和非法指令回显。
3. 验证双机地址过滤、载荷边界、`+++`、忙/超时恢复和连续接收。
4. 修改射频参数后重新上电，确认预期的参数保持行为。
5. 使用合适仪器和当地允许的参数测量频率、输出功率、电流及射频性能。

在记录这些测试前，本固件只能称为“构建已验证”，不能称为“硬件已验证”。
