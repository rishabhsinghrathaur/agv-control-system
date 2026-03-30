# Hardware Setup Guide

## Components List

### Core Components
- **Teensy 4.1** (PJRC) - Main motor controller
- **Raspberry Pi 5** (4GB/8GB) - High-level control
- **4× BLDC Hub Motors** - E-bike style wheel hub motors
- **4× BLDC Controllers** - Typically 48V-72V e-bike controllers
- **MPU6050** - 6-axis gyro/accelerometer (I2C)
- **2× HC-SR04** - Ultrasonic distance sensors
- **USB Camera** -any compatible UVC camera
- **Power Supply** - Appropriate for motors + electronics
- **A lot of wires** - And connectors!

### Optional Components
- **RPLidar A1/A2** - For SLAM and mapping
- **Wheel encoders** - For odometry and position tracking
- **XBee/NRF24L01** - Wireless telemetry
- **LED indicators** - Status/error/warning
- **Buzzer** - Audible alerts
- **Emergency stop button** - Hardware kill switch
- **Battery management system (BMS)** - For Li-ion packs

---

## Pinout Reference

### Teensy 4.1 Pin Assignments

| Pin | Function | Note |
|-----|----------|------|
| GPIO28 | Throttle FL | PWM @ 20kHz |
| GPIO29 | Throttle FR | PWM @ 20kHz |
| GPIO25 | Throttle RL | PWM @ 20kHz |
| GPIO24 | Throttle RR | PWM @ 20kHz |
| GPIO5  | Brake FL | Active LOW |
| GPIO4  | Brake FR | Active LOW |
| GPIO2  | Brake RL | Active LOW |
| GPIO3  | Brake RR | Active LOW |
| GPIO8  | Reverse FL | Direction |
| GPIO9  | Reverse FR | Direction |
| GPIO6  | Reverse RL | Direction |
| GPIO7  | Reverse RR | Direction |
| TX1 (GPIO14) | UART TX to RPi | 3.3V logic |
| RX1 (GPIO15) | UART RX from RPi | 3.3V logic |

**Default Pin State:** All brakes ON (LOW), throttles at 0

**Power Requirements:** 5V (USB) or external 5V regulator

---

### Raspberry Pi 5 Pin Assignments

| Pin | Function | Note |
|-----|----------|------|
| GPIO14 (TX) | UART TX to Teensy | Pin 8 |
| GPIO15 (RX) | UART RX from Teensy | Pin 10 |
| GPIO23 | Ultrasonic Trigger Front | Output |
| GPIO24 | Ultrasonic Echo Front | Input |
| GPIO17 | Ultrasonic Trigger Back | Output |
| GPIO27 | Ultrasonic Echo Back | Input |
| GPIO2 | MPU6050 SDA | I2C1 SDA |
| GPIO3 | MPU6050 SCL | I2C1 SCL |

**UART Configuration:**
```bash
# Disable serial console in /boot/firmware/config.txt
# or use raspi-config:
sudo raspi-config
# Interface Options → Serial Port
# - Disable serial console
# - Enable serial port
```

**USB Devices:**
- Camera: `/dev/video0` (typically)
- Teensy USB Serial: `/dev/ttyACM0` or `/dev/ttyUSB0`

---

### MPU6050 Connections (RPi)

| MPU6050 Pin | Raspberry Pi Pin |
|-------------|------------------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO3 (Pin 5) |
| SDA | GPIO2 (Pin 3) |
| AD0 | GND (address 0x68) |

**Pull-up resistors:** Usually built into MPU6050 module (4.7kΩ to 3.3V)

---

### HC-SR04 Ultrasonic Connections (×2)

**Front Sensor:**
- VCC → 5V (RPi Pin 2)
- Trig → GPIO23 (Pin 16)
- Echo → GPIO24 (Pin 18) **WITH 1kΩ voltage divider!**
- GND → GND (Pin 6)

**Back Sensor:**
- VCC → 5V (RPi Pin 4)
- Trig → GPIO17 (Pin 11)
- Echo → GPIO27 (Pin 13) **WITH 1kΩ voltage divider!**
- GND → GND (Pin 9)

**⚠️ IMPORTANT:** HC-SR04 Echo pin outputs 5V. Use a simple voltage divider (2 resistors: 1kΩ + 2kΩ) to step down to 3.3V for RPi GPIO OR use a logic level shifter.

---

### BLDC Controller Connections (×4)

Each controller typically has:
- **Power:** +48V/72V from battery pack, GND
- **Motor:** 3 phase wires to hub motor (U, V, W)
- **Signal:** Throttle (0-5V or PWM), Brake (active HIGH or LOW), Reverse (optional)

**Wiring per corner:**
```
[Teensy] → [BLDC Controller]
---------------------------
28/29/25/24 → Throttle signal (PWM 0-5V)
5/4/2/3 → Brake (LOW = brake ON)
8/9/6/7 → Reverse (direction control)

[Power] → [Controllers]
[N+ Battery] → + power terminal
[N- Battery] → - power terminal (also GND)
```

**Brake Logic:**
- Most e-bike controllers: Brake active when signal pulled LOW to GND
- Check your controller datasheet - some use active HIGH (toggle with `digitalWrite(brake, HIGH)`)

**Throttle Signal:**
- PWM @ 20kHz with 8-bit resolution (0-255)
- Throttle values: STOP=0, RUN=128, MAX=255

---

### Power Distribution

**Option A: Separate Power**
- Motors: High-current battery pack (48V-72V)
- Teensy + RPi: DC-DC buck converter 12V→5V (or 5V USB-C)

**Option B: Common Ground**
- All GNDs must be connected together (battery -, controllers, Teensy, RPi)
- Use star topology to avoid ground loops

**Current Requirements:**
- 4× BLDC motors: 20-60A each (peak)
- Teensy: <100mA
- RPi 5: 0.5-1A (typical), up to 3A with peripherals
- MPU6050, sensors: ~50mA total

---

## Assembly Order

1. **Solder all connections** - Double-check continuity
2. **Wire power first** - Battery → controllers, DC-DC → 5V rail
3. **Connect signal lines** - Teensy → controllers, RPi → sensors
4. **Verify voltages** - No shorts, correct polarity
5. **Test without motors** - Verify PWM signals with oscilloscope or multimeter
6. **Arm BLDC controllers** - Throttle minimum during power-on
7. **Test motor response** - One at a time via serial commands
8. **Integrate sensors** - MPU6050, ultrasonics
9. **Test obstacle avoidance** - With manual control
10. **Enable Web UI** - Verify Flask server and camera
11. **Low-speed testing** - In open area, emergency stop ready
12. **Calibration** - Trim, gyro, turn precision

---

## Safety Checklist

- [ ] All high-voltage wires properly insulated
- [ ] Fuse on battery positive terminal
- [ ] Emergency stop switch cuts motor power (not just signals)
- [ ] No loose wires that can short
- [ ] Brakes functional before powering motors
- [ ] LiPo packs in fire-resistant container
- [ ] Area clear during first motor test
- [ ] Have remote kill-switch ready (unplug UART? cut power?)

---

## Troubleshooting

### Teensy Issues
| Problem | Likely Cause | Fix |
|---------|--------------|-----|
| No USB enumeration | Bad cable/ solder joint | Check 5V VBUS, D+/- |
| Motors twitch randomly | PWM frequency mismatch | Set PWM to 20kHz (adjust code) |
| Direction commands reversed | Wire swap on REVERSE pins | Swap HIGH/LOW logic in code |
| Brakes not engaging | Active HIGH vs LOW | Invert digitalWrite logic |

### Raspberry Pi Issues
| Problem | Likely Cause | Fix |
|---------|--------------|-----|
| UART permission denied | Not in `dialout` group | `sudo usermod -a -G dialout $USER` |
| MPU6050 not found | Wrong I2C address | Check voltage, run `i2cdetect -y 1` |
| Camera black screen | Wrong device index | Change `cv2.VideoCapture(0)` to 1 or 2 |
| gpio warnings | Using BCM vs BOARD mismatch | Ensure `GPIO.setmode(GPIO.BCM)` |

---

## Bill of Materials (Example)

| Item | Qty | Estimated Cost (USD) | Notes |
|------|-----|---------------------|-------|
| Teensy 4.1 | 1 | $30 | PJRC.com |
| Raspberry Pi 5 | 1 | $60 | 4GB or 8GB |
| BLDC Hub Motor | 4 | $80-150 | E-bike style, with tire |
| BLDC Controller | 4 | $40-80 | 48V 25A minimum |
| MPU6050 Module | 1 | $5 | GY-521 breakout |
| HC-SR04 | 2 | $3 | Ultrasonic sensor |
| USB Camera | 1 | $20 | Logitech C270 or similar |
| Power Distribution Board | 1 | $15 | For 48V battery |
| DC-DC Converter | 1 | $15 | 48V→5V, 5A |
| Wires, connectors, etc. | - | $50 | Heat shrink, terminals |
| **Total** | - | **$500-800** | Depends on motor quality |

---

## Advanced Extensions

1. **RPLidar Integration:** Connect via USB, use `rplidar-ros` or `pyRPLIDAR`
2. **ROS2 Bridge:** Implement `ros2_control` nodes on RPi
3. **Battery Monitor:** ADS1115 ADC to measure pack voltage
4. **CAN Bus:** Use Teensy's CAN1/CAN2 for industrial comms
5. **RTK GPS:** PX4 or u-blox ZED-F9P for outdoor localization

---

For wiring diagrams and circuit schematics, see `schematics/` directory.
