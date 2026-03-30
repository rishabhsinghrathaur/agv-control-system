# Communication Protocol Reference

## Teensy 4.1 Firmware Protocol

The Teensy firmware supports two command modes:

### 1. PID Real-Time Mode (Recommended)

Designed for Raspberry Pi integration with smooth, continuous control without blocking delays.

#### Drive Command: `d<value>`

Continuous forward/backward movement with steering correction.

**Format:** `d<integer>` where -50 to +50

**Examples:**
- `d0` → Drive straight (no steering correction)
- `d15` → Steer right (more power to right wheels)
- `d-10` → Steer left (more power to left wheels)

**Logic:**
```cpp
leftSpeed  = THROTTLE_RUN + correction;
rightSpeed = THROTTLE_RUN - correction + BASE_TRIM_RIGHT;
```

**Notes:**
- Both wheels on same side receive same speed
- Correction is additive (positive = right, negative = left)
- Right trim bias applied automatically
- No braking applied - use `s` to stop separately

---

#### Turn Command: `t<value>`

Continuous in-place rotation (skid-steer turn).

**Format:** `t<integer>` where -255 to +255

**Examples:**
- `t50` → Turn right at speed 50
- `t-50` → Turn left at speed 50
- `t120` → Faster turn right
- `t-200` → Fast turn left

**Logic:**
```cpp
if turnSpeed > 0:
    setRight()  // Left wheels forward, right wheels reverse
else:
    setLeft()   // Right wheels forward, left wheels reverse

finalSpeed = constrain(THROTTLE_STOP + abs(turnSpeed), 0, 255);
setAllThrottles(finalSpeed);
```

**Notes:**
- Uses differential steering (wheels on each side turn opposite)
- Speed determines turn rate; tune based on surface/friction
- Requires sufficient torque to overcome static friction
- Positive = clockwise (right turn), Negative = counterclockwise

---

### 2. Legacy Manual Mode

For direct manual control via serial terminal.

#### Forward: `f`
- Sets all wheels forward direction
- Applies `THROTTLE_RUN` speed (129)
- 150ms delay for synchronized start

#### Backward: `b`
- Sets all wheels reverse direction
- Applies `THROTTLE_RUN` speed (129)
- 150ms delay for synchronized start

#### Stop: `s`
- Sets all throttles to 0
- 100ms delay
- Engages all brakes (active LOW - sets pins LOW)

---

## Serial Interface

### Dual Serial Ports

Teensy 4.1 listens on **both** serial interfaces simultaneously:
- `Serial` - USB CDC (connects to PC/RPi as `/dev/ttyACM0`)
- `Serial1` - Hardware UART (pins 0/1, cross-connected to RPi GPIO14/15)

**Purpose:**
- `Serial` → Debug console / manual control
- `Serial1` → Automated RPi control

### Command Format

```
<command_char><optional_value>\n
```

**Examples:**
```
d15\n
t-30\n
s\n
f\n
```

**Parsing:**
```cpp
char cmd = input[0];
int value = 0;
if (input.length() > 1) {
    value = input.substring(1).toInt();  // Handles negative numbers
}
```

---

## Raspberry Pi Interface

The Python code provides a wrapper around the serial protocol.

### Serial Sender

```python
def send_command(cmd: str):
    """Send command to Teensy via UART."""
    if ser:
        ser.write((cmd + "\n").encode())
        print(f"📡 {cmd}")
```

### High-Level Functions

**drive_continuous(correction):**
```python
# Sends drive command with steering correction
# Used by Flask API for manual steering
send_command(f"d{correction}")
```

**execute_turn(direction):**
```python
# direction: 'l' or 'r'
# Uses MPU6050 to turn by exact angle (TURN_TARGET degrees)
# Sends turn command continuously while monitoring gyro
send_command(direction)  # 'l' or 'r' - legacy commands
# but chassis.py expects 'l' or 'r' and uses intermediate speed
```

### Flask Web API

**GET /** → Web UI with controls and video feed

**POST /cmd/<command>**
```
/cmd/f  → forward (hold)
/cmd/b  → backward (hold)
/cmd/s  → stop
/cmd/l  → turn left 15°
/cmd/r  → turn right 15°
```

**GET /status** → JSON status
```json
{
  "front": false,
  "back": false
}
```

**GET /video_feed** → MJPEG stream

---

## Timing & Performance

### Command Latency

```
RPi → UART → Teensy: ~1-2ms
Teensy execution: <1ms (PID mode, no delays)
```

### UART Baud Rate

115200 baud = ~11520 bytes/sec
- Command `"d15\n"` = 4 bytes → 0.35ms transmission time
- No bottleneck for real-time control at 50-100Hz

### PWM Frequency

20kHz (20,000 Hz)
- Period: 50μs
- 8-bit resolution: steps of 78μs (1/255 of period)

---

## Command Timing Examples

### Autonomous Maneuver: Forward → Turn 90° → Forward

```python
# Send to UART
send_command("d0")          # Drive straight
time.sleep(2.0)             # Move forward 2 seconds

send_command("t50")         # Turn right at speed 50
wait_for_gyro_angle(90)     # ~1-2 seconds depending on TURN_SPEED_ADJUST

send_command("d0")          # Drive straight again
```

### Manual Obstacle Avoidance

```python
user_direction = 'f'  # User holds forward button

# Ultrasonic loop monitors every 100ms
# If obstacle detected:
if user_direction == 'f' and obs_front:
    send_command("s")  # Stop
```

---

## Error Handling

### Teensy

- Invalid commands → ignored
- Out-of-range values → constrained to 0-255 for throttle, -50/50 for drive correction
- UART buffer overflow → possible if commands sent >1kHz (shouldn't happen)

### Raspberry Pi

- UART timeout = 0.1s
- Serial port not available → `ser = None`, commands silently dropped
- MPU6050 not found → turn functions no-op, but web UI still works
- Camera failure → video feed shows error frame

---

## Debugging Tips

### Monitor UART Traffic

```bash
# RPi side
sudo cat /dev/serial0 | tee uart.log

# Teensy side (USB)
screen /dev/ttyACM0 115200
```

### Test from Raspberry Pi without Web UI

```python
import serial
ser = serial.Serial('/dev/serial0', 115200)
ser.write(b"d20\n")   # Drive with right bias
time.sleep(2)
ser.write(b"s\n")     # Stop
```

### Test Teensy standalone

Open Arduino Serial Monitor at 115200 baud, type:
```
d0
t30
s
f
b
```

---

## Protocol Extensions

Future commands can be added to `moveVehicle()`:

```cpp
case 'v':  // Set speed (0-255)
  setAllThrottles(value);
  break;

case 'p':  // PID speed control
  pidSetpoint(value);
  break;

case 'g':  // Get sensor data (battery voltage, etc.)
  Serial.println(getBatteryVoltage());
  break;
```

Corresponding RPi Python:
```python
elif cmd == 'v':
    send_command(f"v{value}")
```

---

## Reference Card

| Command | Mode | Value Range | Description |
|---------|------|-------------|-------------|
| `d` | PID | -50 to +50 | Drive with steering correction |
| `t` | PID | -255 to +255 | Turn in place (continuous) |
| `f` | Legacy | None | Forward (fixed speed) |
| `b` | Legacy | None | Backward (fixed speed) |
| `s` | All | None | Stop + brake |
| `l` | Legacy | None | Turn left (gyro-based) |
| `r` | Legacy | None | Turn right (gyro-based) |

**Default Speeds:**
- `THROTTLE_RUN = 129` (50% duty)
- Trim bias `BASE_TRIM_RIGHT = 5` (added to right side)

---

Last Updated: 2026-03-30
