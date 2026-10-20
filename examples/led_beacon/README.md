# Milestone 4.4 / 4.5 LED 示例

适用课程实验板与普通 RP2040 Pico，使用 GPIO11、14、15 三组红绿 LED，
不使用 GP25 板载 LED。需先按讲义完成 LED 与限流电阻的装配。

启动后全部绿灯亮 2 秒、全部红灯亮 2 秒、全部熄灭 2 秒，然后三组灯
每隔 1 秒切换颜色。完整红绿周期为 2 秒。

`led.h` 声明接口，`led.c` 实现控制，`main.c` 演示两个 milestone。
只接受三组灯的 GPIO 编号，其他参数被忽略。全部熄灭通过输入高阻状态、
关闭上下拉实现；不是输出低电平。从全灭状态调用 toggle 会先点亮绿色。
请在主循环中调用这些接口，不要同时从中断或第二个核控制同一组引脚。

当前讲义的图 4.9 缺失，文字仅说明高、低电平分别点亮一种颜色。
代码暂定 HIGH 为绿、LOW 为红；实机若相反，把 `led.h` 中
`LED_GREEN_LEVEL` 改为 `0` 后重新编译。高阻熄灭亦需在课程板上验证。

在仓库根目录编译（需已安装 Pico SDK、CMake 和 ARM GCC 工具链）：

```bash
cmake -S examples/led_beacon -B build-led-beacon \
    -DPICO_SDK_PATH=/你的路径/pico-sdk
cmake --build build-led-beacon -j
```

此 CMake 文件使用仓库里的 `tools/bin_to_uf2.py`，请保留目录结构。
如果已有独立 Pico SDK 工程，可复制三个 C/H 文件，然后在已初始化 SDK 的
CMakeLists.txt 中使用：

```cmake
add_executable(led_beacon main.c led.c)
target_link_libraries(led_beacon pico_stdlib hardware_gpio)
pico_add_extra_outputs(led_beacon)
```

按住 BOOTSEL 插入 Pico，松开按钮，将 `build-led-beacon/led_beacon.uf2`
复制到 RPI-RP2 挂载目录。成功烧录后磁盘会消失，示例开始执行。
本示例会替换 Pico 当前运行的程序。

验收：确认启动时颜色顺序正确、三组均可完全熄灭、后续每秒红绿交替。
编译通过不能代替实际接线和灯光效果验证。
