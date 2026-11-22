# 第九章：Pico 内部温度传感器

按讲义 9.5 节实现 `adc_initialise()` 和 `uint32_t adc_getTemp()`。
使用内部 ADC 通道 4，不需要外部传感器。每秒更新 LCD 和 USB 串口。
LCD 复用 `../lcd/lcd.c`：GP0 为 PWM 对比度，GP1 为 E，GP2 为 RW，
GP3 为 RS，GP4–7 为 D4–D7。串口使用 USB CDC，禁用 GPIO UART。

换算：`voltage = raw * 3.3 / 4096`，
`temperature = 27 - (voltage - 0.706) / 0.001721`。
遵循讲义的无符号整数接口，返回四舍五入后的整数摄氏度，负温度返回 0。
这是芯片内部温度，只能粗略反映温度，不能作为精确室温测量。

在仓库根目录使用服务器已有依赖构建：

```sh
export PATH="$PWD/.deps/python/cmake/data/bin:$PWD/.deps/arm-gnu-toolchain-14.2.rel1-aarch64-arm-none-eabi/bin:$PATH"
cmake -S examples/adc -B build-adc -DPICO_SDK_PATH="$PWD/.deps/pico-sdk"
cmake --build build-adc -j4
```

固件输出为 `build-adc/adc_temperature.uf2`。
按住 BOOTSEL 插入 Pico，把 UF2 复制到 `RPI-RP2` 磁盘。
正常运行后，LCD 第二行显示 `Temp: ... C`；USB 串口每秒输出一行温度。
可以在串口终端选择 Pico 的 USB CDC 端口，以 115200 打开查看。
编译验证不能代替实机温度与 LCD 显示验收。
