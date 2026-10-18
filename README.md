# PEMSY 双向无线寻呼机

根据 `script_english.pdf`（Version 6.2）第 1.9 节和第 12 章实现的完整课程项目。
目标硬件：**RP2040 Raspberry Pi Pico、课程 PEMSY PCB、16×2 HD44780 LCD、旋转编码器、RFM69W**。

实现旋钮文字输入、选择收件人/广播、双向无线消息、LCD、PWM 对比度、ADC 芯片温度、
可选 mioty 上报、发送超时恢复和 mioty 掉电计数器。核心应用使用 C11，mioty 适配层使用 C++17。
无需 RTOS 或动态应用内存分配。软件已完成主机测试和 ARM 交叉编译；**尚未进行实物射频/基站验收**。

## 快速使用

本次构建产物：

- `build-node1/pemsy.uf2`：节点 1，初始接收目标 2。
- `build-node2/pemsy.uf2`：节点 2，初始接收目标 1。
- `build-mioty/pemsy.uf2`：启用 mioty 的编译验证产物，使用零密钥示例，不能直接接入实验室基站。
- `build-host/pemsy_simulator`：可在这台服务器上运行的交互式状态机模拟器。

上述 `build-*` 目录由构建生成，不进入 Git；全新克隆后按下文重新构建。
两个普通固件默认采用讲义第 1 组 **869.80 MHz、RFM69W、−8 dBm**。
按住 Pico 的 BOOTSEL 插入 USB，将对应 UF2 复制到 `RPI-RP2` 磁盘即可刷写。
两块板必须使用不同节点号、相同频率组。无线使用范围按课程要求执行。

在收件箱短按旋钮选择收件人，再短按进入文本输入。旋转选择字符，短按添加，
长按 3 秒保存，在确认页短按发送。最多 16 字符，支持可打印 ASCII。
在收件箱旋转可进入温度或对比度页面。完整说明见 [操作说明](docs/usage.md)。

## 构建环境

需要 CMake 3.21+（预设功能）、Make 或 Ninja、Python 3、Git、主机 C/C++ 编译器，
以及 `arm-none-eabi-gcc` / `arm-none-eabi-g++` 和 Pico SDK。
本次验证版本为 Pico SDK **2.1.1**、Arm GNU Toolchain **14.2.Rel1**，主机 GCC **11.5**。
工具链必须匹配构建电脑架构，本服务器是 Linux aarch64。

```sh
# 克隆本项目时使用 --recurse-submodules；已有目录执行：
git submodule update --init

# 将 SDK 安装到项目忽略的依赖目录（只需一次）
git clone --depth 1 --branch 2.1.1 https://github.com/raspberrypi/pico-sdk.git .deps/pico-sdk
git -C .deps/pico-sdk submodule update --init --depth 1 lib/tinyusb
export PICO_SDK_PATH="$PWD/.deps/pico-sdk"
# 将已安装的 Arm 工具链 bin 目录加入 PATH。
arm-none-eabi-gcc --version
```

官方资料：[Pico SDK](https://github.com/raspberrypi/pico-sdk)、
[Arm 工具链安装](https://learn.arm.com/install-guides/gcc/arm-gnu/)。
构建不会自动下载 SDK、picotool 或密钥。项目自带 RP2040 专用 UF2 转换工具，
输出格式依据 [UF2 规范](https://github.com/microsoft/uf2)，并检查固件不能覆盖计数器区域。

```sh
cmake --preset node1
cmake --build --preset node1 -j4
cmake --preset node2
cmake --build --preset node2 -j4
```

可配置参数：

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `PEMSY_NODE_ADDRESS` | 1 | 本机节点号，1–254 |
| `PEMSY_DEFAULT_RECIPIENT` | 2 | 初始收件人，1–254 或广播 255 |
| `PEMSY_GROUP` | 1 | 讲义频率组 1–6，见 [无线协议](docs/radio.md) |
| `PEMSY_RFM69_HIGH_POWER` | OFF | HW/HCW 模块设为 ON；W/CW 为 OFF |
| `PEMSY_ENABLE_MIOTY` | OFF | 启用带端点配置的 mioty |
| `PEMSY_MIOTY_CONFIG_HEADER` | `config/mioty_credentials.h` | 本地端点凭据文件 |
| `PEMSY_ENCODER_EDGES_PER_DETENT` | 4 | 每定位点边沿数，可设 2 或 4 |

例如第 3 组、节点 7 向节点 8 通信：

```sh
cmake -S . -B build-custom -DPEMSY_GROUP=3 \
  -DPEMSY_NODE_ADDRESS=7 -DPEMSY_DEFAULT_RECIPIENT=8
cmake --build build-custom -j4
```

## 启用 mioty

```sh
cp config/mioty_credentials.example.h config/mioty_credentials.h
# 编辑该文件：填入基站已注册的 Network Key、EUI64、Short Address。
cmake --preset mioty
cmake --build --preset mioty -j4
```

本地凭据文件已被 Git 忽略。示例中的全零密钥仅用于构建测试；讲义表 11.2/11.3
给出了课程参数，需与实验室实际登记一致。两块板上报时应分别配置不同的端点身份。
如果曾用 `-DPEMSY_MIOTY_CONFIG_HEADER=...` 测试示例，正式构建时应显式改为真实配置文件路径，
因为 CMake 会保留旧的缓存值。

温度页短按触发一次上报，两次尝试至少间隔 60 秒。单块 RFM69 不能同时运行
对讲和 mioty：上报期间暂时停止对讲接收，第二核心处理 mioty，主核保持 UI 响应。
完成后自动恢复对讲接收。软件的发送完成不代表对端或基站已接收。

## 测试与模拟器

```sh
cmake --preset host
cmake --build --preset host -j4
ctest --preset host
./build-host/pemsy_simulator
```

九组测试覆盖应用状态机、计数器断电/边界、编码器、ADC、LCD 协议、无线驱动、
真实 TS-UNB MAC/PHY 加模拟 SPI/时钟，以及 UF2 文件格式。
驱动测试使用硬件接口替身，不模拟真实射频传播。主机测试应使用 Debug 构建，
测试目标也显式保持断言有效。

实机接线、两板互发、温度上报与掉电验收见 [硬件与验收](docs/hardware.md)。
构建和测试证据见 [验证记录](docs/validation.md)。

## 代码结构与要求对应

| 讲义要求 | 实现 |
| --- | --- |
| 1.9 / 12 完整无线寻呼机 | `src/main.c` 集成与 `src/app.c` 状态机 |
| 6 LCD 单字符、多行、自定义字符 | `src/lcd.c`，含 CGRAM API 和协议测试 |
| 7 4kHz / 1.5% 对比度 PWM | LCD 驱动与对比度菜单 |
| 8 旋转检测、消抖、短按/长按、文本输入 | `src/encoder.c`、`src/app.c` |
| 9 ADC 内部温度 | `src/temperature.c`，采样平均和温度页面 |
| 10 SPI、自检、中断、寻址收发 | `src/SPI.c`、`src/radio.c` |
| 11 mioty 温度上报及 C/C++ 交互 | `src/mioty.cpp`、`include/pemsy/mioty.h` |
| 12 错误和断电处理 | 有界无线超时、重试、看门狗、`src/counter*.c` |

课程提到的 `rfm69c.c` 未包含在输入文件中，因此提供了完整替代驱动和明确的报文格式；
两台运行本固件的设备可以使用同一协议，不能声称兼容未提供的课程驱动。
早期独立 LED 灯组练习没有单独的演示固件，最终系统使用板载 LED 表示通信状态。
草稿、收件箱和对比度存在 RAM，重启恢复默认；只保留最新一条接收消息。

外设中断只采集事件，主核执行 UI 和对讲 SPI。mioty 工作核通过队列获得独占无线访问权。
计数器使用 Flash 最后 8 KiB 双扇区日志，每次预留 256 个数，持久化成功后才允许发送；
断电跳过旧区间，24 位计数器耗尽或日志无法识别时拒绝发送。
正常 UF2 不覆盖日志；全片擦除/恢复旧镜像后必须重新处理基站端点状态。

`TS-UNB-Lib-Pico` 作为固定提交的 Git 子模块保留原实现与许可证。
其学术/非商业使用范围及再分发条件见 [原库许可证](TS-UNB-Lib-Pico/LICENSE.md)。
