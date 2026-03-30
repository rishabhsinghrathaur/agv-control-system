# Future Upgrades Roadmap

This document outlines planned enhancements for the AGV platform, from practical improvements to advanced research features.

---

## Category 1: Immediate Practical Upgrades (Low Effort, High Impact)

### 1.1 Battery Monitoring & Auto-Shutdown

**Problem:** No battery voltage feedback; AGV can deep-discharge LiPo packs.

**Solution:**
- Add ADS1115 ADC (I2C) to measure pack voltage
- Voltage divider with proper scaling (e.g., 4S LiPo: 16.8V max → 3.3V ADC input)
- Teensy ADC pin alternative (but less accurate)

**Implementation:**
```python
# RPi reads voltage via I2C
import adafruit_ads1x15.ads1115 as ADS

def get_battery_voltage():
    # Read ADC channel 0
    voltage = adc.read_adc(0) * (16.8 / 32767)
    return voltage * (R1 + R2) / R2  # voltage divider math

# Send warning/stop if voltage < cutoff
if voltage < 14.0:  # 4S minimum ~14V
    send_command('s')
    print("⚠️ LOW BATTERY - STOPPING")
```

**Priority:** HIGH - protects expensive battery packs

---

### 1.2 Wireless Communication (Remove UART Cable)

**Problem:** UART tether limits AGV mobility and creates tripping hazard.

**Options:**

**A) XBee Series 2 (ZigBee)**
- Range: 100-300m line-of-sight
- Simple UART passthrough
- Create point-to-point or mesh network

**B) NRF24L01+**
- 2.4GHz, 100m range
- Very cheap (~$2/module)
- Requires custom driver on RPi/Teensy

**C) ESP32 as WiFi Serial Bridge**
- Most flexible: TCP/UDP sockets over WiFi
- WiFi Direct or Ad-Hoc mode for no router needed
- Range ~50m

**Recommended:** ESP32 in ad-hoc mode
- RPi and Teensy each have ESP32
- Simple WiFi AP with static IPs
- Python serial over WiFi (`ser = serial.serial_for_url('socket://192.168.4.1:8080')`)

---

### 1.3 Emergency Stop Hardware Button

**Problem:** Web UI stop button depends on network, can lag in emergency.

**Solution:** Hardware E-stop that physically disconnects motor power.

**Implementation:**
- Large red button with NC (normally closed) contacts
- Inline with battery positive terminal
- Or: E-stop triggers Teensy GPIO interrupt → instant brake engagement

**Circuit:**
```
[Battery+] → [E-Stop NC] → [Controllers +]
                  ↓
               [GPIO] - Pulled up, interrupt on FALLING_EDGE
```

**Teensy Code:**
```cpp
pinMode(ESTOP_PIN, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), estopISR, FALLING);

void estopISR() {
    brakeON();
    setAllThrottles(0);
}
```

**Priority:** CRITICAL for safety

---

### 1.4 LED Status Indicators

Mount visible LEDs on AGV chassis for at-a-glance status:

| LED | Color | Meaning |
|-----|-------|---------|
| Power | Green | 5V rail OK |
| System | Blue | RPi running, API healthy |
| Bluetooth/WiFi | Blue (blink) | Connected to controller |
| Error | Red | Any fault (low batt, sensor fail) |
| Moving | Yellow | Motors energized |

**Wiring:** GPIO pins with 330Ω resistors, driven by RPi or Teensy

---

### 1.5 Improved Odometry

**Problem:** No position estimation - AGV drifts without sensors.

**Solution:** Add wheel encoders.

**Options:**
- Hall effect sensors + magnets on wheel hubs
- Optical encoders (IR slot type)
- Quadrature encoders → count direction

**Integration:**
- Teensy has 4× encoder inputs (pins 0-23 support)
- Use `Encoder` library
- Track pulses → calculate distance traveled

**Code:**
```cpp
#include <Encoder.h>
Encoder encFL(32), encFR(33), encRL(34), encRR(35);

void loop() {
    long posFL = encFL.read();
    // Calculate velocity, distance, heading
    // Send to RPi via serial: "O<left_dist>,<right_dist>\n"
}
```

---

## Category 2: Advanced Control & Autonomy

### 2.1 PID Speed Control

**Current State:** Open-loop throttle commands; motor speed varies with terrain.

**Goal:** Maintain exact wheel speed regardless of incline/slope.

**Implementation:**

**Teensy Side:**
- Read wheel encoders (velocity feedback)
- PID controller per wheel or per side
- Adjust PWM output to maintain setpoint

```cpp
// Per-side PID
float leftSetpoint = 0.5;  // m/s
float leftEncoderCounts = readEncoderLeft();
float leftSpeed = countsToVelocity(leftEncoderCounts);
float leftOutput = pidLeft.compute(leftSetpoint, leftSpeed);
analogWrite(THROTTLE_FL, leftOutput);
analogWrite(THROTTLE_RL, leftOutput);
```

**RPi Side:**
- Send desired velocity: `v<speed_in_mps>` or `d<speed><correction>`
- Teensy handles PID loops at 1kHz

**Priority:** HIGH - essential for precise movement

---

### 2.2 Absolute Heading with Compass

**Problem:** MPU6050 gyro drifts over time; heading drifts during turns.

**Solution:** Add magnetometer (HMC5883L or QMC5883L) for absolute heading.

**Wiring:** I2C shared with MPU6050 (different address)

**Sensor Fusion:**
- Complementary filter: gyro (fast) + magnetometer (slow, absolute)
- Or use Madgwick/Mahony filter

**Use Cases:**
- Maintain exact heading while driving
- Navigate to absolute compass bearings
- Dead reckoning

---

### 2.3 Pure Pursuit Path Following

**Goal:** Follow a pre-defined path (series of waypoints) autonomously.

**Algorithm:**
- Pure Pursuit: lookahead point on path, steer toward it
- Stanley Controller: minimize cross-track error

**Implementation:**
1. Define path as list of (x, y) coordinates
2. Estimate position using odometry (encoders + gyro)
3. Compute control: curvature → steering correction
4. Send `d<correction>` to Teensy

**RPi Code Structure:**
```python
class PurePursuit:
    def __init__(self, lookahead=0.5):
        self.lookahead = lookahead  # meters

    def compute(self, pose, path):
        # pose: (x, y, theta)
        # path: [(x1,y1), (x2,y2), ...]
        target = self.find_lookahead_point(pose, path)
        curvature = self.calculate_curvature(pose, target)
        correction = self.curvature_to_throttle_bias(curvature)
        return correction
```

---

### 2.4 SLAM and Mapping

**Goal:** Build a map of the environment and localize within it.

**Hardware:** Add **RPLidar A1/A2** (360° 2D LIDAR, 5-10m range)

**Software:**
- ROS2 (recommended) with SLAM Toolbox
- Or python + `slam_toolbox` or `cartographer`
- Or `python_breeze` (lighterweight)

**Data Flow:**
- RPLidar USB → RPi reads scan data at 5-10Hz
- Process scans → occupancy grid map
- Fuse with odometry → localization

**Output:**
- `/map` topic (occupancy grid)
- `/amcl` pose estimate
- Navigation stack can then send `d<correction>` commands

**Complexity:** HIGH - requires heavy computation, careful integration

---

### 2.5 ROS2 Integration

**Goal:** Make AGV ROS2-native for interoperability with research code.

**Architecture:**

```
[RPLidar] ──→ [RPi ROS2] ──→ /cmd_vel (geometry_msgs/Twist)
                              ↓
                        [Teensy ROS2] ──→ Motor control
```

**Implementation:**

**RPi ROS2 Node:**
- Publishes `/scan` from LIDAR
- Publishes `/odom` from wheel encoders + IMU
- Subscribes to `/cmd_vel` (teleop or navigation stack)
- Serial bridge to Teensy

**Teensy ROS2 (micro-ROS):**
- micro-ROS on Teensy 4.1
- Subscribes to motor commands
- Publishes encoder feedback, IMU data

**Benefits:**
- Use `ros2_control` for standard motor controller interface
- Plug-and-play with Nav2, SLAM Toolbox, MoveIt
- Community ecosystem

---

## Category 3: Sensing & Perception

### 3.1 AprilTag / ArUco Fiducial Navigation

Use camera to detect AprilTags placed in environment for:
- Precise positioning
- Landmark-based localization
- Dock alignment

**Implementation:**
```python
import apriltag
detector = apriltag.Detector()
tags = detector.detect(gray_image)
# Get tag pose relative to camera
# Fuse with odometry
```

**Applications:**
- Autonomous docking to charging station
- Specific waypoint navigation (e.g., "go to tag 3")
- Map annotation

---

### 3.2 Line Following (Simple AGV Mode)

Add IR reflectance sensors underneath for following tape on floor.

**Hardware:** 3-5x TCRT5000 or QRE1113 sensors

**Algorithm:**
- PID line following: sensor centroid → steering correction
- Simple and reliable for warehouse/manufacturing applications

**Integration:**
- Place sensors in front of AGV, ~2-3cm from ground
- Teensy reads analog values (use ADC), computes line offset
- Alternately: RPi reads camera bottom row, simpler but heavier

---

### 3.3 Depth Camera for Obstacle Avoidance

**Replace** HC-SR04 (single point) with **Intel RealSense D435** or **Orbbec Astra**.

**Benefits:**
- 3D point cloud
- Detect obstacles at multiple heights
- See over small obstacles (e.g., cables, small bumps)
- Better navigation in cluttered spaces

**Use Cases:**
- Table/dock detection
- Stair detection (if camera angled down)
- 3D obstacle map for Nav2

**Trade-off:** Higher power consumption, USB bandwidth

---

### 3.4 LiDAR-Based Navigation

As mentioned in SLAM section, RPLidar enables:
- 2D mapping (walls, furniture)
- Localization
- Dynamic obstacle avoidance (Nav2 `costmap_2d`)

**Alternative:** Sweep LiDAR (better range, higher cost)

---

## Category 4: Power & Safety

### 4.1 BMS Integration

For Li-ion/LiPo packs exceeding 4S (14.8V+), need BMS for:
- Cell balancing
- Over-current protection
- Temperature monitoring
- Charge/discharge control

**Communication:**
- JST-SYP (SMBus) → I2C to RPi
- ANT BMS (CAN bus) → Teensy CAN1

**Integration:**
- RPi reads voltage, current, temperature, SOC
- Emergency stop if over-current or over-temp
- Display battery stats on web UI

---

### 4.2 Fused Power Distribution

Design proper power distribution board:
- Separate fuses for motors, logic, sensors
- Soft-start for ESCs (prevent inrush)
- Common ground star topology
- Power-on sequencing (logic first, then enable motors)

**Tool:** PCBWay or custom PCB instead of breadboard/wire harness

---

### 4.3 Buzzer & Audible Alerts

Simple piezo buzzer for:
- Startup chime
- Low battery warning (beep pattern)
- Obstacle warning (fast beeps)
- Emergency stop (continuous tone)

**Wiring:** GPIO → transistor → buzzer (if loud) or direct GPIO if small

---

## Category 5: Mechanical & Chassis

### 5.1 Custom Chassis Design

If stock AGV chassis inadequate:
- Design in Fusion 360 / SolidWorks
- CNC cut from aluminum or polycarbonate
- Integrate motor mounts, sensor towers, wire management

**Design Requirements:**
- Wheelbase ~500mm, track width ~400mm (typical)
- Center of gravity low (battery in middle)
- Skid plates for impact protection
- Mounting points for sensors

---

### 5.2 Wheel Encoder Discs + Magnets

If using Hall effect encoders:
- 3D-print encoder disc with embedded magnets
- Mount to motor shaft (may need coupler)
- Calibrate counts per revolution

**Factory Alternative:** Buy encoder-ready BLDC motors (~$150 ea)

---

### 5.3 Suspension (Optional)

For uneven terrain:
- Add small springs or elastomer mounts on wheel axles
- Improves traction, reduces chassis stress
- Complicates odometry (wheel travel affects encoder counts)

---

## Category 6: Software & Infrastructure

### 6.1 Configuration Management

**Problem:** Calibration values scattered across two files.

**Solution:** Central config file (JSON/YAML):

```yaml
# config.yaml
motors:
  trim_right: 5
  throttle_run: 129
  pwm_freq: 20000

gyro:
  turn_target: 15
  turn_speed_adjust: 0.9

ultrasonics:
  obstacle_dist_cm: 70
  pins:
    front_trigger: 23
    front_echo: 24
    back_trigger: 17
    back_echo: 27

uart:
  port: /dev/serial0
  baud: 115200
```

Both Teensy (with LittleFS) and RPi read same file.

---

### 6.2 Remote Management Dashboard

Build advanced web UI with:
- Real-time telemetry (battery, speed, errors)
- Calibration sliders (live tune trim, PID gains)
- Log viewer
- File manager (logs, maps)
- SSH terminal in browser

**Tools:** Flask + Socket.IO for real-time updates, Bootstrap for UI

---

### 6.3 Logging & Analytics

Log all sensor data and commands to file for later analysis:

```python
import csv
with open('log.csv', 'a') as f:
    writer = csv.writer(f)
    writer.writerow([time.time(), cmd, left_speed, right_speed, gyro_z, ...])
```

Use for:
- Tuning PID parameters
- Debugging weird behavior
- Performance metrics (battery life, average speed)

**Rotation:** Daily logs, compress old ones

---

### 6.4 Docker Deployment

Containerize RPi application:

```dockerfile
FROM python:3.9-slim
RUN apt-get update && apt-get install -y libgl1-mesa-glx
COPY . /app
RUN pip install -r requirements.txt
CMD ["python", "/app/pythonfile.py"]
```

Benefits:
- Consistent environment
- Easy deployment
- Isolation from RPi OS updates

---

## Category 7: Research & Experimental

### 7.1 Deep Learning-Based Navigation

Train CNN to:
- Classify objects (person, box, chair)
- Predict free space
- End-to-end driving (pixels → steering)

**Given constraints:** RPi 5 can run TensorFlow Lite on CPU (~5-10 FPS).

**Workflow:**
1. Collect dataset of camera images + steering commands
2. Train model (on workstation)
3. Convert to TFLite
4. Deploy to RPi: capture frame → infer correction → send `d<value>`

---

### 7.2 Multi-AGV Coordination

Multiple AGVs communicating via WiFi/MQTT:
- Task allocation
- Traffic management (avoid collisions)
- Formation control (platoon)

**Protocol:** MQTT topics:
- `agv/1/pose`
- `agv/1/cmd`
- `agv/1/status`

**Central Orchestrator:** RPi or cloud server assigns tasks

---

### 7.3 Swarm Behavior

Decentralized coordination:
- Each AGV broadcasts state via UDP multicast
- Local collision avoidance using neighbor positions
- Emergent flocking, leader-follower

**Algorithms:** Boids, Virtual Structures, Behavior-Based

---

### 7.4 Arm Integration

Attach small robotic arm (e.g., UFactory xArm, DIY with Dynamixel servos):

**Tasks:**
- Pick and place
- Object manipulation
- Charging dock plug-in

**Integration:**
- Teensy controls arm via serial (Dynamixel bus)
- RPi plans pickup/place trajectories

---

## Quick Reference: Prioritization

| Upgrade | Effort | Impact | Priority |
|---------|--------|--------|----------|
| E-stop button | 1h | CRITICAL | MUST DO |
| Battery monitor | 2h | HIGH | MUST DO |
| WiFi serial bridge | 4h | HIGH | SOON |
| Wheel encoders | 4h | HIGH | SOON |
| PID speed control | 8h | HIGH | SOON |
| ROS2 integration | 16h | HIGH | LATER |
| RPLidar SLAM | 24h | HIGH | LATER |
| AprilTags | 4h | MED | LATER |
| Depth camera | 2h | MED | OPTIONAL |
| Swarm coordination | 40h | RESEARCH | OPTIONAL |

---

## How to Implement Upgrades

1. **Create feature branch:**
   ```bash
   git checkout -b feature/<name>
   ```

2. **Update hardware/software incrementally**
   - Test each component isolated
   - Integrate with existing system
   - Document changes

3. **Update docs:**
   - `HARDWARE.md` with new connections
   - `protocol.md` with new commands
   - `upgrades.md` mark as implemented ✅

4. **Commit & PR:**
   ```bash
   git add .
   git commit -m "feat: <upgrade description>"
   git push origin feature/<name>
   ```

---

## Questions?

Open an issue or refer to `docs/troubleshooting.md`
