# 验证记录

验证日期：2026-10-18。当前环境没有连接 Pico、RFM69、LCD 或 mioty 基站。

## 已执行的软件验证

`cmake --preset host`、`cmake --build --preset host`、`ctest --preset host`：
**9/9 测试组通过**，主机编译启用 `-Wall -Wextra -Werror -pedantic`。

| 测试组 | 验证内容 |
| --- | --- |
| application | 旋钮输入、16 字符边界、广播、发送结果、异常消息、温度、对比度、时间回绕 |
| persistent_counter | 连续计数、重启跳号、跨扇区、逐字节中断写入、擦除失败 |
| counter_limits | 24 位耗尽、无效记录、损坏日志、读取失败、写入确认丢失 |
| encoder | Gray 转移、机械抖动、短按/长按、事件转移、时间回绕 |
| temperature | ADC 输入选择、丢弃首样本、16 次平均、公式换算 |
| lcd | 初始化半字节序列、两行裁剪/补空格、CGRAM、4kHz PWM/对比度 |
| radio | 芯片缺失、FIFO、自定义报文、地址/CRC/长度检查、发送超时、RX 恢复 |
| mioty_adapter | 实际库 MAC/PHY、模拟 SPI/时钟、计数器限制、发送模式、迟到处理 |
| uf2_format | 数据回读、块头/家族 ID、块序号、长度与保留 Flash 边界 |

同一组测试另用 **AddressSanitizer + UndefinedBehaviorSanitizer** 执行，9/9 通过。
设置 `ASAN_OPTIONS=detect_leaks=0`，因为当前环境不支持 LeakSanitizer 所需的进程检查；
因此没有宣称完成内存泄漏检测。

```sh
cmake -S . -B build-sanitize -DPEMSY_HOST_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build-sanitize -j4
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-sanitize --output-on-failure
```

终端模拟器已执行输入 `HI`、向节点 2 模拟发送、接收 `HELLO` 的完整流程。

## 固件构建

使用 Pico SDK 2.1.1、Arm GNU Toolchain 14.2.Rel1，Release 配置。
以下配置均完成 ARM 编译与链接，并输出 `.elf`、`.bin`、`.uf2`：

| 输出目录 | 配置 |
| --- | --- |
| build-node1 | RFM69W，组 1，节点 1 → 2，mioty 关闭 |
| build-node2 | RFM69W，组 1，节点 2 → 1，mioty 关闭 |
| build-mioty | RFM69W，组 1，节点 1 → 2，mioty 启用、示例凭据 |
| build-hw-mioty | RFM69HW，组 6，节点 254 → 广播 255，mioty 启用、示例凭据、两边沿编码器 |

mioty 测试构建使用 **全零示例密钥**，不是实验室正式配置。
链接器为应用保留 2040 KiB Flash，最后 8 KiB 留给计数器；工作核栈从普通 RAM
分配 16 KiB，避免默认 4 KiB scratch 区容不下 mioty 工作栈。

这些构建产物验证代码、依赖、接口与链接兼容性，不能验证 PCB 接线、LCD 电压、
旋钮机械定位、频率误差、射频灵敏度或基站解码。实机步骤见 [硬件验收](hardware.md)。
