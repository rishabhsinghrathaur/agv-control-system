# Pinout Reference Quick Card

## Raspberry Pi 5 GPIO Pinout (BCM Numbering)

```
        3V3  1│  │2  5V
        GPIO2 3│  │4  5V
        GPIO3 5│  │6  GND
        GPIO4 7│  │8  GPIO14 (TXD1)
         GND  9│  │10 GPIO15 (RXD1)
       GPIO17 11│  │12 GPIO18
       GPIO27 13│  │14 GND
       GPIO22 15│  │16 GPIO23
          3V3 17│  │18 GPIO24
       GPIO10 19│  │20 GND
        GPIO9 21│  │22 GPIO25
       GPIO11 23│  │24 GPIO8
         GND 25│  │26 GPIO7
```

**Used Pins:**
- **GPIO2, 3** → MPU6050 SDA/SCL (I2C1)
- **GPIO14, 15** → UART TX/RX to Teensy
- **GPIO23** → HC-SR04 Front Trigger
- **GPIO24** → HC-SR04 Front Echo (via voltage divider)
- **GPIO17** → HC-SR04 Back Trigger
- **GPIO27** → HC-SR04 Back Echo (via voltage divider)

---

## Teensy 4.1 Pinout (Digital)

```
Digital Pins (all 3.3V):
28 → Throttle FL (PWM)
29 → Throttle FR (PWM)
25 → Throttle RL (PWM)
24 → Throttle RR (PWM)

5  → Brake FL (OUT, active LOW)
4  → Brake FR (OUT, active LOW)
2  → Brake RL (OUT, active LOW)
3  → Brake RR (OUT, active LOW)

8  → Reverse FL (DIR)
9  → Reverse FR (DIR)
6  → Reverse RL (DIR)
7  → Reverse RR (DIR)

14 → UART1 TX → RPi GPIO15
15 → UART1 RX ← RPi GPIO14
```

---

## MPU6050 Pinout

```
MPU6050    →   RPi
─────────────────────
VCC       →   3.3V
GND       →   GND
SCL       →   GPIO3 (Pin 5)
SDA       →   GPIO2 (Pin 3)
AD0       →   GND (for I2C addr 0x68)
```

---

## HC-SR04 Pinout (×2)

```
HC-SR04     →   RPi
─────────────────────────────
VCC        →   5V
Trig       →   GPIO (see above)
Echo       →   GPIO via voltage divider (1kΩ + 2kΩ)
GND        →   GND
```

**Voltage Divider:**
```
Echo (5V) → [1kΩ] → GPIO
GPIO      → [2kΩ] → GND
Result: ~3.3V at GPIO
```

---

## BLDC Controller Typical Connections

```
          48V Battery ──→ [BLDC Controller]
                                  │
              Throttle ──────┐   └─→ 3 phase wires → Motor
           Brake (if separate)┘
              Reverse (optional)
```

**Teensy Connections (per controller):**
- **Throttle signal** (PWM) → Teensy GPIO pin
- **Brake** (if controllable) → Teensy GPIO pin
- **Reverse** (if available) → Teensy GPIO pin
- **Ground** → common GND with Teensy 3.3V/5V logic?

---

## Power Connections Summary

```
[Battery Pack] (48-72V)
       │
       ├──→ [BLDC Controller ×4] ───→ Motors
       │
       └──→ [DC-DC Converter] (48V → 5V, 5A+)
                │
                ├──→ Teensy 4.1 (USB 5V or 5V pin)
                └──→ Raspberry Pi 5 (USB-C 5V 3A+)
```

**Important:**
- All grounds (Battery -, Controller -, Teensy, RPi) must be connected together
- Use star topology (all connect to single point) to avoid ground loops

---

## Wiring Colors (Suggested Convention)

- **Red**: +5V
- **Black**: GND
- **Green**: Enable/Signal
- **Yellow**: Direction/Reverse
- **Blue**: Brake

---

## Quick Check Before Power-On

- [ ] No shorts between 5V and GND
- [ ] No shorts between 5V and 48V
- [ ] Teensy and RPi share common GND
- [ ] All throttle pins at 0V (use multimeter)
- [ ] Ultrasound Trig pins at 0V (idle)
- [ ] Battery polarity correct (+ to +)
- [ ] Fuse in series with battery positive

---

**See `HARDWARE.md` for complete installation guide.**
