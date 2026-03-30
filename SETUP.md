# Complete Setup Guide

This guide walks through setting up the AGV from scratch with a fresh Raspberry Pi 5 and Teensy 4.1.

---

## Table of Contents

1. [Raspberry Pi 5 Preparation](#raspberry-pi-5-preparation)
2. [Teensy 4.1 Firmware Upload](#teensy-41-firmware-upload)
3. [Hardware Wiring](#hardware-wiring)
4. [Software Installation](#software-installation)
5. [Initial Testing](#initial-testing)
6. [Calibration](#calibration)
7. [Web Interface](#web-interface)
8. [Common Issues](#common-issues)

---

## Raspberry Pi 5 Preparation

### 1. Flash Raspberry Pi OS

1. Download **Raspberry Pi OS Lite (64-bit)** or **Desktop** from raspberrypi.com
2. Use Raspberry Pi Imager to flash to SD card
3. Before first boot, add:
   - Enable SSH (touch `ssh` file on boot partition)
   - Set up WiFi (add `wpa_supplicant.conf`)
   - Configure locale/timezone

### 2. Initial Boot

1. Boot Pi, connect via SSH:
   ```bash
   ssh pi@raspberrypi.local
   # default password: raspberry
   ```

2. Change password:
   ```bash
   passwd
   ```

3. Update system:
   ```bash
   sudo apt update && sudo apt upgrade -y
   sudo reboot
   ```

### 3. Enable Interfaces

```bash
sudo raspi-config
```

Navigate to:
- **Interface Options** → **Serial Port**
  - Disable serial console
  - Enable serial port

- **Interface Options** → **Camera**
  - Enable camera (if using, though we use USB camera)

- **Interface Options** → **I2C**
  - Enable I2C (for MPU6050)

### 4. Clone Repository

```bash
cd ~
git clone <your-repo-url>
cd agv-project
```

### 5. Install Python Dependencies

```bash
# Update package list
sudo apt update

# Install system dependencies
sudo apt install -y python3-pip python3-venv libmpi-openmpi-dev \
    i2c-tools libatlas-base-dev libjpeg-dev libopenjp2-7-dev \
    zlib1g-dev libavcodec-dev libavformat-dev libswscale-dev

# Install Python packages
pip3 install -r requirements.txt

# Add user to necessary groups
sudo usermod -a -G gpio,dialout $USER
```

**Log out and back in** for group changes to take effect.

---

## Teensy 4.1 Firmware Upload

### 1. Install Arduino IDE

1. Download from [arduino.cc](https://www.arduino.cc/en/software)
2. Install:
   ```bash
   tar -xzf arduino-*.tar.xz
   sudo mv arduino-* /opt/arduino
   sudo ln -s /opt/arduino/arduino /usr/local/bin/arduino
   ```

### 2. Install Teensyduino

1. Download [Teensyduino](https://www.pjrc.com/teensy/td_download.html)
2. Run installer, select:
   - Arduino IDE install path (`/opt/arduino`)
   - Install USB drivers
   - Install Teensyduino (check all libraries)

### 3. Upload Firmware

1. Open Arduino IDE
2. **File** → **Open** → `teensy41.ino`
3. Configure board:
   - **Tools** → **Board** → **Teensy 4.1**
   - **Tools** → **USB Type** → **Serial**
   - **Tools** → **CPU Speed** → **600 MHz (overclock)** or **528 MHz**
4. Connect Teensy via USB
5. Click **Verify** → **Upload**

**Expected Console Output:**
```
--- System Initialized ---
PID Commands: d<val> (Drive), t<val> (Turn in place)
```

---

## Hardware Wiring

Follow the detailed pinout in [HARDWARE.md](HARDWARE.md). Here's a quick checklist:

### Raspberry Pi 5

- [ ] MPU6050 connected to GPIO2 (SDA), GPIO3 (SCL), 3.3V, GND
- [ ] Front ultrasonic: GPIO23 (Trig), GPIO24 (Echo with voltage divider)
- [ ] Back ultrasonic: GPIO17 (Trig), GPIO27 (Echo with voltage divider)
- [ ] USB camera connected
- [ ] UART connections ready (GPIO14/15)

### Teensy 4.1

- [ ] Throttle pins (28, 29, 25, 24) connected to BLDC controllers
- [ ] Brake pins (5, 4, 2, 3) connected
- [ ] Reverse pins (8, 9, 6, 7) connected
- [ ] USB connected to RPi (optional, for debugging)
- [ ] UART1 cross-connected to RPi (TX1→RX, RX1→TX)

### Power

- [ ] Battery pack connected to BLDC controllers
- [ ] DC-DC converter connected to battery (output 5V @ 5A+)
- [ ] Teensy powered via USB from RPi **OR** external 5V
- [ ] Raspberry Pi powered via USB-C PD (5V 3A minimum)
- [ ] All GNDs connected together

---

## Software Installation (Detailed)

### 1. Configure UART

Disable serial console to free UART:

```bash
sudo systemctl stop serial-getty@ttyAMA0.service
sudo systemctl disable serial-getty@ttyAMA0.service
sudo nano /boot/firmware/cmdline.txt
```

Remove any occurrences of `console=serial0,115200` from the line.

Save and reboot.

### 2. Test UART Connection

```bash
# List serial devices
ls -la /dev/tty*

# Test with minicom (install if needed)
sudo apt install -y minicom
sudo minicom -b 115200 -o -D /dev/serial0
```

You should see the Teensy initialization message. Press `Ctrl+A`, then `Q` to quit.

### 3. Test MPU6050

```bash
# Install i2c-tools if not already
sudo apt install -y i2c-tools

# Scan I2C bus
sudo i2cdetect -y 1

# Should see device at address 0x68
```

### 4. Test Ultrasonic Sensors

```bash
python3 -c "
import RPi.GPIO as GPIO
import time
TRIG, ECHO = 23, 24
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)
GPIO.output(TRIG, False)
time.sleep(0.1)
GPIO.output(TRIG, True)
time.sleep(0.00001)
GPIO.output(TRIG, False)
start = time.time()
while GPIO.input(ECHO)==0:
    start = time.time()
while GPIO.input(ECHO)==1:
    stop = time.time()
distance = (stop-start) * 17150
print(f'Distance: {distance:.1f} cm')
GPIO.cleanup()
"
```

---

## Initial Testing

### Without Motors (Signal Test)

1. **Monitor Teensy Serial** (via USB or UART):
   ```bash
   screen /dev/ttyACM0 115200
   # or
   cat /dev/serial0
   ```

2. Send test commands:
   ```
   d0      # Should show no movement, command received
   d50     # Test drive command (motors off but signal sent)
   t30     # Test turn command
   s       # Stop
   ```

3. **Measure PWM with multimeter/oscilloscope**:
   - Check throttle pin (e.g., GPIO28)
   - Should see ~20kHz signal (~2-3V average when "running")

### With Motors Connected (BE READY TO CUT POWER!)

1. Ensure area is clear, motors can spin freely
2. Power on battery
3. Send `f` command via serial → all motors should spin forward
4. Send `b` command → all motors reverse
5. Send `s` → brakes engage

---

## Calibration

### Trim Adjustment (Straight Driving)

1. Place AGV on low-friction surface (smooth floor)
2. Command `f` (forward)
3. Observe direction:
   - **Drifts right** → increase `BASE_TRIM_RIGHT` in `teensy41.ino` (try 10-15)
   - **Drifts left** → decrease `BASE_TRIM_RIGHT` (try 0-3)
4. Re-upload firmware, repeat

**For PID mode** (`d` command):
```cpp
int leftSpeed  = THROTTLE_RUN + correction;
int rightSpeed = THROTTLE_RUN - correction + BASE_TRIM_RIGHT;
```

### Gyro Calibration

The code automatically calibrates on startup (100 samples over 1 second).

**To manually re-calibrate:**
- Keep Teensy still for 1 second after power-on
- Or modify code to call `calibrate_gyro()` on command

### Turn Precision

Adjust `TURN_SPEED_ADJUST` in `pythonfile.py`:

```python
TURN_SPEED_ADJUST = 0.9  # Lower = more precise, turns slower
# Try: 0.85, 0.88, 0.9, 0.93, 0.95
```

Test with the web UI 15° turn buttons:
- Measure turns with protractor or mark floor
- If consistently **undershooting** (12° instead of 15°) → increase value
- If consistently **overshooting** (17° instead of 15°) → decrease value

---

## Web Interface

### Run Server

```bash
cd ~/agv-project
python3 pythonfile.py
```

Expected output:
```
✅ UART Connected
✅ MPU Connected
 * Running on http://0.0.0.0:5000
```

### Access from Phone/Laptop

1. Find RPi IP:
   ```bash
   hostname -I
   # e.g., 192.168.1.42
   ```

2. Open browser to `http://192.168.1.42:5000`

3. Test controls:
   - Hold ▲ → AGV moves forward, release → stops
   - Click ◀ 15° → spins left by ~15°
   - Observe obstacle warnings (front/back)

### Auto-start on Boot (Optional)

```bash
sudo nano /etc/systemd/system/agv.service
```

```ini
[Unit]
Description=AGV Control Server
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/agv-project
ExecStart=/usr/bin/python3 /home/pi/agv-project/pythonfile.py
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable agv
sudo systemctl start agv
sudo systemctl status agv
```

---

## Common Issues

### "UART Connected: False"

- Check `ls -la /dev/serial0` → should point to `ttyAMA0`
- Verify UART enabled in `raspi-config`
- Check wiring: GPIO14 (TXD1) → Teensy RX1, GPIO15 (RXD1) ← Teensy TX1

### "MPU Connected: False"

- Check `i2cdetect -y 1` shows `68`
- Verify MPU6050 powered with 3.3V or 5V
- Check SDA/SCL wiring (GPIO2/3)

### "Permission denied for /dev/serial0"

```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Camera Not Working

```bash
# List devices
v4l2-ctl --list-devices

# Test camera
ffplay -f v4l2 -framerate 30 -video_size 640x480 /dev/video0
```

### Camera Black Screen in Web UI

- Ensure OpenCV installed correctly: `python3 -c "import cv2; print(cv2.__version__)"`
- Check camera is not used by another process (e.g., motion detection)
- May need to specify different camera index: `cap = cv2.VideoCapture(1)`

---

## Next Steps

After basic setup:
1. **Calibrate trim** for straight driving
2. **Adjust turn precision** for accurate 15° turns
3. **Test obstacle avoidance** with soft obstacles first
4. **Mount everything** securely on AGV chassis
5. **Implement PID speed control** (extend `teensy41.ino`)
6. **Add features** from `docs/upgrades.md`

---

## Support

- Check `docs/troubleshooting.md` for detailed issue resolution
- Verify all wiring against `HARDWARE.md`
- Test components individually before full integration
