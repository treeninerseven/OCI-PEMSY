# LCD 自定义图案（6.8）

在已有的 LCD 练习上，用 8 个 5×8 自定义字符拼出参考图片中的 mioty 图案。
程序占用全部 8 个 CGRAM 槽位，图案大小为 20×16 点，位于两行的第 7–10 列。
这是单色近似，字符格之间会有间隙。

- `lcd.c`：保留已有通信、字符串输出和定位函数，新增 `lcd_createChar()`。
- `mioty_logo.c`：8 块点阵及 `lcd_show_mioty_logo()`，先写 CGRAM 再写 DDRAM。
- `main.c`：初始化后显示图案；图案显示时关闭光标和闪烁。
- `preview.py`：从 `mioty_logo.c` 读取同一份点阵，生成模拟效果图 `preview.png`。

每个自定义字符由 8 个字节组成；每个字节低 5 位对应一行像素，bit 4 在最左侧。
槽位 0 也必须通过单字符函数写入：用字符串传送会把 0 当作字符串结束符。
`lcd_createChar()` 写完 CGRAM 后会把输出位置设回左上角，随后可用 `lcd_locate()` 定位。
现有 `lcd_writedate()` 的名称保留，以便沿用课堂练习。

## 构建

安装 Pico SDK、ARM 工具链和 CMake 后，在仓库根目录运行：

```sh
cmake -S examples/lcd -B build-lcd-logo -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build-lcd-logo -j2
```

生成的固件是 `build-lcd-logo/lcd_logo.uf2`，适用于课程 RP2040 Pico 板。
UART 标准输出已关闭，避免占用 LCD 的 GP0/GP1。

程序使用课程板原有引脚，不加入第七章的 PWM 任务。
实际显示仍需要板上的 LCD 供电、接线及对比度电压正确。
效果图仅模拟点阵和字符间隙，不能代替实物验证。

## 重新生成效果图

```sh
python examples/lcd/preview.py
```

此脚本需要 Pillow。修改 `mioty_logo.c` 的点阵后重新运行即可。
