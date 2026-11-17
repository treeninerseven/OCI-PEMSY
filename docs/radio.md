# Radio protocol and mioty integration

SPI0 uses MISO GP16, software CS GP17, SCK GP18, MOSI GP19, mode 0,
4 MHz. RFM69 DIO0 connects to GP20. The shared GPIO interrupt handler must call
`radio_handle_irq`; only the main loop touches SPI. Call `radio_poll` regularly.
The receiver defaults to RX, briefly visits standby before every RX/TX change,
and returns to RX after transmission. Mode changes have a 25 ms deadline and TX
has a 250 ms deadline. A missing radio fails initialization rather than hanging.
Initialization also performs the 16-byte FIFO SPI loopback milestone.

The course's `rfm69c.c` was absent from the supplied files. This replacement
therefore defines its own explicit protocol; it is interoperable with a second
board running this firmware, not guaranteed compatible with an unspecified
course driver. Both peers must use the same group frequency and different IDs.

| Group | Frequency (Hz) |
| --- | --- |
| 1 (default) | 869800000 |
| 2 | 869820000 |
| 3 | 869840000 |
| 4 | 869860000 |
| 5 | 869880000 |
| 6 | 869900000 |

Set `PEMSY_NODE_ADDRESS` to 1–254; destination 255 is broadcast.
Set CMake's `PEMSY_GROUP` to 1–6; it generates `PEMSY_RADIO_FREQUENCY_HZ`
from the six table 10.1 frequencies.
`PEMSY_RFM69_HIGH_POWER=0` selects the course RFM69W (PA0, -8 dBm).
For HW/HCW set it to 1 (PA1, -2 dBm minimum); these modules cannot transmit at
-8 dBm on their antenna output. mioty uses the same hardware selection.

The peer air interface is 4800 bit/s binary FSK, approximately ±5 kHz deviation,
10.4 kHz receiver bandwidth, three-byte preamble, sync `2D D4`, no whitening or
Manchester coding. The chip generates and checks its standard two-byte CRC.
Bad CRC packets are automatically discarded. FIFO bytes are:

| Field | Bytes | Meaning |
| --- | --- | --- |
| Length | 1 | Number of subsequent bytes, 3–18 |
| Destination | 1 | Node ID or 255 |
| Source | 1 | Sender node ID, 1–254 |
| Text | 1–16 | Printable ASCII bytes, no transmitted NUL |

The chip filters destination addresses. The driver checks length and source,
terminates text in a 17-byte buffer, and retains one unread packet. Later packets
are discarded while that slot is occupied. Transmission completion confirms
the radio finished sending; there is no acknowledgement or automatic retry.

Register settings and interrupt mapping were checked against the manufacturer's
[RFM69HW datasheet](https://www.hoperf.com/uploads/RFM69HW-V1_3Datasheet_1702458847.pdf),
sections 5.3, 5.5 and 6. The matching RFM69W module is used by the course.

## Optional mioty

The default firmware builds without credentials or mioty transmission. Enabling
`PEMSY_ENABLE_MIOTY` needs a private `config/mioty_credentials.h`, copied from the
example and filled with the registered endpoint's network key, EUI64 and short
address. Alternatively define `PEMSY_MIOTY_CONFIG_HEADER` to an external header.
The example's zero key intentionally cannot authenticate to the lab gateway.
Transmit power defaults to the course's 10 dBm. Follow the supplied course's lab
frequency/access requirements and register the endpoint before testing.

The adapter uses the supplied library's unmodified MAC, PHY and RFM69 templates
with EU1 settings. A small Pico platform implementation replaces the upstream
alarm flag loop with absolute hardware-clock deadlines. It also converts the
upstream standby/FS mode numbers into the register's bits 4:2. Each wait is
bounded, the entire scheduled transmission is capped at 60 seconds, and timing
lateness beyond 2 ms reports failure. These paths need physical radio/gateway
validation; a successful software return does not confirm gateway receipt.

Use the application's second-core worker for the blocking mioty call. Before
handoff call `radio_suspend`, and do not call any peer/SPI function until the
worker completes. The worker calls `mioty_setup`, `mioty_set_counter` with a
counter already persisted by the application, then `mioty_ep_to_base`. Every
attempt consumes that counter. Reserve the next counter before another send;
reusing a counter after power loss would violate replay protection. Values above
24 bits are refused. On completion (including failure) the main core calls
`radio_init` to restore every packet-mode setting and RX.

The adapter accepts 1–16 text bytes, including the application's formatted ADC
temperature. It is uplink only; peer RX is unavailable during mioty transmission.
The supplied library retains its academic/non-commercial license; see
`TS-UNB-Lib-Pico/LICENSE.md` before redistributing or using outside the lab.
