# Troubleshooting Guide

Comprehensive solutions for common problems in the AGV system.

---

## Table of Contents

1. [Serial Communication](#serial-communication)
2. [Motor Control](#motor-control)
3. [Sensors](#sensors)
4. [Web Interface](#web-interface)
5. [Power Issues](#power-issues)
6. [Performance Tuning](#performance-tuning)
7. [Error Codes](#error-codes)

---

## Serial Communication

### Symptom: "UART Connected: False" on RPi startup

**Possible Causes & Solutions:**

1. **UART not enabled**
   ```bash
   sudo raspi-config
   # Interface Options → Serial Port
   # ✓ Enable serial port
   # ✗ Disable serial console
   sudo reboot
   ```

2. **Wrong device path**
   ```bash
   # List serial devices
   ls -la /dev/serial0  # Should point to ttyAMA0
   ls -la /dev/ttyAMA0  # Actual UART device

   # If missing, check config.txt
   sudo nano /boot/firmware/config.txt
   # Ensure: enable_uart=1
   ```

3. **Permission denied**
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```

4. **Wiring issue**
   - Teensy TX1 (GPIO14) → RPi RX (GPIO15)
   - Teensy RX1 (GPIO15) → RPi TX (GPIO14)
   - Common GND between boards
   - Cross crossover, not straight-through

5. **Baud rate mismatch**
   - Both sides must use 115200
   - Teensy: `Serial1.begin(115200);`
   - RPi: `serial.Serial('/dev/serial0', 115200)`

---

### Symptom: Commands sent but no action

**Check:**

1. **Correct port being used**
   ```bash
   # Monitor UART
   sudo cat /dev/serial0
   # Type: d20
   # Check Teensy USB serial monitor shows command
   ```

2. **Teensy running correct firmware**
   - Open Arduino Serial Monitor (115200)
   - Should see: `--- System Initialized ---`
   - If not, re-upload `teensy41.ino`

3. **Command format**
   - Must end with `\n` (newline)
   - `send_command("d20")` automatically adds `\n`
   - Manual terminal: type `d20` then **Enter**

---

## Motor Control

### Symptom: Motors don't spin, just beep or twitch

**Likely Cause:** BLDC controller not armed.

**Solution:**
1. Ensure throttle signal at **0V** during power-on
2. Many controllers require 1-3 seconds with zero throttle to arm
3. Check controller manual for specific arming procedure (beep sequence)

**Test:**
```cpp
// In teensy41.ino setup(), before sending any commands:
setAllThrottles(THROTTLE_STOP);  // = 0
delay(2000);  // Wait 2s for ESCs to arm
```

---

### Symptom: One motor spins opposite direction

**Cause:** Motor phase wires swapped (U/V/W order) OR REVERSE pin logic inverted.

**Solution:**

**Option A - Swap any 2 phase wires** on that motor's controller (e.g., U↔V).

**Option B - Change direction logic** in `teensy41.ino`:

```cpp
void setForward() {
  digitalWrite(REVERSE_FR, HIGH);  // Was LOW
  digitalWrite(REVERSE_RR, LOW);   // Was HIGH
  // ... adjust per motor
}
```

**Option C - Reverse wire to REVERSE pin** (swap with another corner)

---

### Symptom: Motor spins but weak / stalls under load

**Causes:**

1. **Insufficient battery voltage**
   - LiPo 4S: nominal 14.8V, min ~14V, full 16.8V
   - Check under load with multimeter
   - Sag below controller minimum (often 36V for 48V controller)?

2. **PWM frequency mismatch**
   - Some ESCs expect 50Hz (R/C servos) not 20kHz
   - Check controller documentation
   - Change in `teensy41.ino`:
     ```cpp
     #define PWM_FREQ 50   // For 50Hz servos
     ```

3. **Throttle signal range wrong**
   - 8-bit (0-255) for 20kHz
   - Some ESCs expect 1-2ms pulse (50Hz) - need servo library
   - Verify with oscilloscope

4. **Thermal protection**
   - Controller/ motor overheating → throttled
   - Check for error LEDs on controller

---

### Symptom: Vehicle drifts to one side (PID mode)

**Solution:** Adjust `BASE_TRIM_RIGHT` in `teensy41.ino`.

```cpp
// Test:
// 1. Place AGV on level ground, mark starting point
// 2. Send: d0
// 3. Drive 5 meters
// 4. Measure drift

// If drifts RIGHT → increase BASE_TRIM_RIGHT
#define BASE_TRIM_RIGHT 15  // Try 10, 15, 20

// If drifts LEFT → decrease BASE_TRIM_RIGHT
#define BASE_TRIM_RIGHT 0   // Or even -5 (negative correction on right)
```

**Advanced:** Trim individual corners by modifying:

```cpp
int leftSpeed  = THROTTLE_RUN + correction + TRIM_LEFT;
int rightSpeed = THROTTLE_RUN - correction + BASE_TRIM_RIGHT + TRIM_RIGHT;
```

---

### Symptom: Brakes don't engage

**Check:**

1. **Brake logic** (active HIGH or LOW)?
   - Most e-bike controllers: brake ON when signal pulled LOW (to GND)
   - If yours is opposite, invert in code:
     ```cpp
     void brakeON() {
       digitalWrite(BRAKE_FL, HIGH);  // Was LOW
       // ...
     }
     ```

2. **Brake wire connection**
   - Some controllers have separate brake output wire (small connector)
   - Others use throttle signal: brake = 0V on throttle signal wire
   - Consult controller manual

3. **Brake power**
   - Many e-bike brakes require external 12V power (from battery)
   - Check brake has its own power connection

---

### Symptom: Jerky movement, not smooth

**Causes:**

1. **Too coarse PWM resolution**
   - 8-bit (256 steps) is coarse at low speeds
   - Teensy can do 16-bit PWM (but 20kHz max at 8-bit)
   - Use higher `PWM_RES` if controller supports (16?)

2. **PID mode using 'd' command should be much smoother** than legacy 'f/b'
   - Ensure using `d<value>` not `f`

3. **Serial commands too slow**
   - Check RPi code isn't blocking
   - Web UI: `send('f')` on mousedown, `send('s')` on mouseup

---

## Sensors

### Symptom: MPU6050 not detected (`mpu = None`)

**Check:**

1. **I2C connection**
   ```bash
   sudo i2cdetect -y 1
   ```
   Should see `68` at address 0x68.

2. **Wiring**
   - MPU6050 VCC → 3.3V (NOT 5V unless module has regulator)
   - MPU6050 GND → GND
   - SCL → GPIO3 (pin 5)
   - SDA → GPIO2 (pin 3)
   - AD0 → GND (for address 0x68)

3. **Pull-up resistors**
   - MPU6050 modules typically have built-in 4.7kΩ pull-ups
   - If not, add 2.2kΩ-10kΩ from SDA→3.3V, SCL→3.3V

4. **Library issues**
   ```bash
   pip3 install mpu6050-raspberrypi
   # Or: pip3 install mpu6050
   ```

   Test standalone:
   ```python
   from mpu6050 import mpu6050
   sensor = mpu6050(0x68)
   print(sensor.get_gyro_data())
   ```

---

### Symptom: Gyro drifts (turns not accurate)

**Causes:**

1. **Poor calibration** - moving during calibration
   - Ensure RPi perfectly still for 1 second during `calibrate_gyro()`
   - Motors off, on flat surface

2. **Vibration noise**
   - Mount MPU6050 on foam or rubber to isolate vibrations
   - Increase calibration samples count (100 → 200)

3. **Temperature drift**
   - MPU6050 bias changes with temperature
   - Recalibrate after warm-up period (5-10 min)

**Solution:** Tune `TURN_SPEED_ADJUST` in `pythonfile.py` to compensate for systematic errors.

---

### Symptom: Ultrasonic readings erratic or always 0/infinite

**Check:**

1. **Trigger pulse**
   ```python
   GPIO.output(trig, True)
   time.sleep(0.00001)  # 10μs pulse
   GPIO.output(trig, False)
   ```
   - Verify with oscilloscope or multimeter (should see brief 5V)

2. **Echo voltage**
   - HC-SR04 Echo = 5V logic
   - **RPi GPIO = 3.3V max - VOLTAGE DIVIDER REQUIRED**
   - Simple divider: Echo → 1kΩ → GPIO, GPIO → 2kΩ → GND
   - Or use logic level shifter

3. **Timeout too short**
   ```python
   timeout = time.time() + 0.05  # 50ms = max ~8.5m
   # Increase to 0.1 for longer range
   ```

4. **Obstruction**
   - Check sensor facing clear
   - Remove any nearby metal objects (can absorb ultrasound)

---

### Symptom: Obstacle detection triggers falsely

**Solution:** Increase `stable_f >= 2` threshold in `ultrasonic_loop()`:

```python
stable_f = stable_f + 1 if d_f < OBSTACLE_DIST else 0
obs_front = stable_f >= 3  # Require 3 consecutive readings
```

**Also:**
- Smoothing: average multiple readings
- Debounce: don't toggle state too rapidly

---

## Web Interface

### Symptom: Page loads but camera shows nothing (black/blank)

**Check:**

1. **Camera accessible**
   ```bash
   # Check camera device
   ls -la /dev/video*
   v4l2-ctl --list-devices

   # Test capture
   ffplay -f v4l2 -video_size 640x480 /dev/video0
   ```

2. **OpenCV installed correctly**
   ```bash
   python3 -c "import cv2; print(cv2.__version__)"
   # Should print 4.x
   ```

3. **Camera in use by another process**
   ```bash
   sudo lsof /dev/video0
   # If motion, guvcview, etc. using it → kill those processes
   ```

4. **Wrong camera index** (if multiple cameras)
   ```python
   cap = cv2.VideoCapture(1)  # Try 1, 2, etc.
   ```

---

### Symptom: Control buttons do nothing

**Check:**

1. **Flask route working**
   ```bash
   # Terminal where pythonfile.py running:
   # Should show: "📡 f" when pressing forward
   ```

2. **Serial connection**
   - `ser` should not be `None`
   - Check `/dev/serial0` permissions
   - Add debug: `print(f"ser.is_open: {ser.is_open if ser else 'None'}")`

3. **Teensy receiving**
   - Open Arduino Serial Monitor on Teensy USB
   - Should see commands appear when pressing buttons

---

### Symptom: Cannot access web UI from phone/laptop

**Check:**

1. **Firewall**
   ```bash
   sudo ufw allow 5000
   # Or disable: sudo ufw disable
   ```

2. **Correct IP address**
   ```bash
   hostname -I
   # Use that IP, not localhost
   ```

3. **Binding address**
   ```python
   app.run(host='0.0.0.0', port=5000)  # 0.0.0.0 = all interfaces
   # NOT: host='127.0.0.1' (localhost only)
   ```

4. **Same network**
   - Ensure phone/laptop on same WiFi as RPi

---

## Power Issues

### Symptom: Random resets / brownouts when motors start

**Cause:** Battery voltage sag from motor current draw affecting RPi/Teensy.

**Solutions:**

1. **Separate power supplies**
   - Motors: Battery direct
   - Logic: Separate DC-DC (or battery + ferrite to isolate)
   - Ensure common GND

2. **Capacitors**
   - Add large electrolytic (2200-10000µF) near motor controllers
   - Small ceramic (.1µF) near Teensy/RPi power pins

3. **Soft-start**
   - Ramp throttle up slowly instead of instant full power
   - In `syncedStart()`: gradually increase PWM

---

### Symptom: USB devices disconnect

**Cause:** USB power sag.

**Solutions:**
- Use powered USB hub for camera, Teensy
- Or separate 5.1V→5V USB-C PD to RPi (better than microUSB)

---

### Symptom: Overheating components

**Check:**
- BLDC controllers: need heatsinks + airflow
- DC-DC converter: adequate current rating?
- RPi: heatsink + fan recommended
- Teensy: normally doesn't overheat

---

## Performance Tuning

### Issue: Turn angle inconsistent (±2-3° error)

**Solutions:**

1. **Calibrate TURN_SPEED_ADJUST**
   - Place angle gauge or mark 90° on floor
   - Test 15° turns repeatedly
   - Adjust value until consistent

2. **Reduce speed for accuracy**
   ```python
   TURN_TARGET = 15
   TURN_SPEED_ADJUST = 0.85  # Lower = slower but more accurate
   ```

3. **Add turning P-controller**
   ```python
   while abs(angle) < TURN_TARGET:
       correction = (TURN_TARGET - abs(angle)) * 0.1
       send_command(f"t{int(turnSpeed * correction)}")
   ```

4. **Surface-dependent**
   - Carpet vs smooth floor changes turn dynamics
   - May need separate tuning profiles

---

### Issue: Straight driving still drifts even with trim

**Possible fixes:**

1. **Check individual motor balance**
   - Each motor/controller may have slight speed variance
   - Measure no-load speed of each wheel, record offsets
   - Hardcode compensation per wheel in Teensy code

2. **Mechanical issues**
   - Wheel diameter mismatch (manufacturing tolerance ~1%)
   - Wheel alignment (camber) causing scrub
   - Weight distribution uneven

3. **Surface slope**
   - Floor not perfectly level?
   - Add gyro-based heading correction in PID mode

---

### Issue: PID mode not smooth, oscillating

**Cause:** Missing or improper PID controller.

**Note:** Current firmware doesn't implement actual PID - just open-loop with trim.

**For true PID:**

```cpp
// In teensy41.ino, add:
float leftSetpoint = 0, leftOutput = 0;
float rightSetpoint = 0, rightOutput = 0;
PID leftPID(&leftEncoderSpeed, &leftOutput, &leftSetpoint, Kp, Ki, Kd);
leftPID.SetMode(1);  // Automatic
leftPID.SetOutputLimits(0, 255);

void loop() {
    // Parse d<value> into setpoint (target speed)
    // Encoder ISR updates encoderCounts
    // PID runs at ~1kHz, updates output
    analogWrite(THROTTLE_FL, leftOutput);
    analogWrite(THROTTLE_RL, leftOutput);
}
```

**Tuning:** Start with Kp only, then add Ki for steady-state, Kd for damping.

---

## Error Codes

### Teensy LED Indicators

If you add status LED to Teensy:

| Blink Pattern | Meaning |
|---------------|---------|
| Solid on | Powered |
| Slow blink (1Hz) | Running, no errors |
| Fast blink (5Hz) | UART receiving data |
| 2 quick blinks, repeat | Serial error / invalid command |
| 3 quick blinks | PID mode active (drive/turn) |

---

### Python Exceptions

**`[Errno 13] Permission denied: '/dev/serial0'`**
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Re-login

**`AttributeError: 'NoneType' object has no attribute 'is_open'`**
- Serial port failed to open
- Check wiring, permissions, device exists

**`ImportError: No module named 'mpu6050'`**
- `pip3 install mpu6050-raspberrypi`

**`cv2.error: OpenCV(4.x.x) ...`**
- OpenCV not properly installed
- Try: `pip3 install --upgrade opencv-python-headless`

---

## Diagnostic Commands

### Check I2C devices:
```bash
sudo i2cdetect -y 1
```

### Check serial devices:
```bash
ls -l /dev/serial*
dmesg | grep tty
```

### Test Teensy firmware:
```bash
# Connect via USB
screen /dev/ttyACM0 115200
# Type: d20
# Should drive if motors connected
```

### Test RPi→Teensy:
```bash
python3 << EOF
import serial, time
ser = serial.Serial('/dev/serial0', 115200)
time.sleep(2)
ser.write(b"d30\n")
print("Sent: d30")
time.sleep(2)
ser.write(b"s\n")
print("Sent: s")
EOF
```

### Test MPU6050:
```bash
python3 << EOF
from mpu6050 import mpu6050
sensor = mpu6050(0x68)
print("Gyro:", sensor.get_gyro_data())
print("Accel:", sensor.get_accel_data())
EOF
```

### Check camera:
```bash
ffmpeg -f v4l2 -video_size 320x240 -i /dev/video0 -vframes 1 test.jpg
display test.jpg
```

---

## Still Stuck?

1. Check wiring **again** (most issues are loose connections)
2. Test components individually (Teensy → serial monitor; RPi → sensors separately)
3. Simplify: run `teensy41.ino` and `pythonfile.py` with **minimal setup** (no extra wires)
4. Add debug prints strategically
5. Search issues in this repository or open a new one

Include:
- Error messages (full traceback)
- Wiring diagram (photo or description)
- What you've already tried
- Components used (exact models)

---

**Last Updated:** 2026-03-30
