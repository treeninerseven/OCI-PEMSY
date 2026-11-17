"""Render the firmware's CGRAM data, including gaps between character cells.

Run: python examples/lcd/preview.py
Requires Pillow. The picture is a simulation, not a hardware photograph.
"""
from pathlib import Path
import re

from PIL import Image, ImageDraw, ImageFont


folder = Path(__file__).resolve().parent
source = (folder / "mioty_logo.c").read_text()
array = source.split("mioty_logo_pixels[8][8] = {", 1)[1].split("};", 1)[0]
values = [int(value, 16) for value in re.findall(r"0x[0-9A-Fa-f]+", array)]
assert len(values) == 64 and all(0 <= value <= 31 for value in values)

image = Image.new("RGB", (1200, 410), "#f5f6f2")
draw = ImageDraw.Draw(image)
try:
    title_font = ImageFont.truetype("DejaVuSans.ttf", 25)
    small_font = ImageFont.truetype("DejaVuSans.ttf", 17)
except OSError:
    title_font = small_font = ImageFont.load_default()

draw.text((45, 24), "MIOTY mark on a 16 x 2 character LCD", font=title_font, fill="#213026")
draw.text((45, 64), "20 x 16 pixels / 8 custom characters / simulated display", font=small_font, fill="#556054")
draw.rounded_rectangle((30, 100, 1170, 375), radius=18, fill="#333b37")
draw.rounded_rectangle((50, 116, 1150, 360), radius=6, fill="#b8cf7c")

for row in range(2):
    for col in range(16):
        slot = row * 4 + col - 6 if 6 <= col < 10 else None
        for y in range(8):
            pattern = values[slot * 8 + y] if slot is not None else 0
            for x in range(5):
                active = (pattern >> (4 - x)) & 1
                left = 74 + col * 66 + x * 11
                top = 128 + row * 117 + y * 13
                draw.rectangle((left, top, left + 8, top + 10),
                               fill="#263c19" if active else "#acc571")

draw.text((45, 385), "Character-cell gaps are shown. Backlight colour and contrast depend on your LCD.",
          font=small_font, fill="#556054")
output = folder / "preview.png"
image.save(output)
print(output)
