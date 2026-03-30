# AGV Implementation Plan

A structured roadmap for implementing upgrades and enhancements to the AGV control system.

---

## 📊 Priorities Overview

| Priority | Upgrade | Effort | Impact | Timeline |
|----------|---------|--------|--------|----------|
| **P0 (CRITICAL)** | E-Stop Hardware | 2h | Safety | Week 1 |
| **P0 (CRITICAL)** | Battery Monitor | 4h | Safety | Week 1-2 |
| **P1 (HIGH)** | Wireless UART Bridge | 8h | Usability | Week 2-3 |
| **P1 (HIGH)** | Wheel Encoders | 6h | Core | Week 3-4 |
| **P1 (HIGH)** | PID Speed Control | 12h | Core | Week 4-6 |
| **P2 (MEDIUM)** | LED Indicators | 2h | UX | Week 6 |
| **P2 (MEDIUM)** | Configuration Management | 4h | Maintainability | Week 6-7 |
| **P3 (ADVANCED)** | Pure Pursuit | 16h | Autonomy | Week 7-9 |
| **P3 (ADVANCED)** | RPLidar SLAM | 24h | Capability | Month 3 |
| **P4 (FUTURE)** | ROS2 Integration | 40h | Ecosystem | Month 4-6 |

---

## 🎯 Milestone 1: Safety & Power (Weeks 1-2) - **P0**

### M1.1: Emergency Stop Hardware

**Status:** Not started
**Dependencies:** None
**Risk:** Low

#### Tasks:
1. **Procurement** (1h)
   - [ ] Buy large red emergency stop button (NC momentary)
   - [ ] Buy appropriate wire gauge (16-18 AWG for battery line)
   - [ ] Buy inline fuse holder (10-20A)

2. **Wiring** (1h)
   - [ ] Install E-stop in series with battery positive
   - [ ] Add indicator LED showing E-stop state (optional)
   - [ ] Ensure E-stop cuts power to BOTH motor controllers AND any control electronics if safety-critical

3. **Teensy Interrupt** (optional, 1h)
   - [ ] Connect E-stop to GPIO pin (e.g., GPIO20)
   - [ ] Add interrupt service routine:
     ```cpp
     volatile bool estop_active = false;
     void estopISR() {
       estop_active = true;
       brakeON();
       setAllThrottles(0);
     }
     ```
   - [ ] Blink status LED when E-stop triggered

4. **Testing** (30min)
   - [ ] Verify E-stop cuts motor power instantly
   - [ ] Verify Teensy detects E-stop (if using interrupt)
   - [ ] Test recovery: release E-stop, restart system

**Success Criteria:**
- [ ] Pressing E-stop immediately stops motors (within 100ms)
- [ ] System cannot be restarted while E-stop engaged
- [ ] E-stop is clearly labeled and accessible

---

### M1.2: Battery Voltage Monitoring

**Status:** Not started
**Dependencies:** None
**Risk:** Medium (high voltage involved)

#### Hardware Options (choose one):
- **Option A:** ADS1115 ADC (I2C, 16-bit) - Recommended
- **Option B:** Teensy ADC pin (12-bit, less accurate)
- **Option C:** Voltage divider to RPi ADC (MCP3008)

#### Tasks (Option A - ADS1115):
1. **Procurement** (1h)
   - [ ] ADS1115 breakout board
   - [ ] Precision resistors for voltage divider (calculate based on battery)
     - Example: 48V pack → divider ratio 1:10 (10kΩ + 1kΩ)
     - Max 50V should scale to 3.3V ADC input

2. **Wiring** (1h)
   ```
   Battery+ → Fuse → [R1=10kΩ] → ADS1115 A0
                              [R2=1kΩ] → GND
   Battery- → GND (common)
   ```
   - [ ] Double-check resistor power rating (P=V²/R)
   - [ ] Add protection diode (TVS) for voltage spikes

3. **I2C Integration** (1h)
   ```bash
   sudo i2cdetect -y 1  # Should see 0x48 (ADS1115 default)
   ```
   - [ ] Install Adafruit CircuitPython ADS1x15 library
     ```bash
     pip3 install adafruit-circuitpython-ads1x15
     ```
   - [ ] Test ADC reading:
     ```python
     import adafruit_ads1x15.ads1115 as ADS
     from board import SCL, SDA
     import busio
     i2c = busio.I2C(SCL, SDA)
     ads = ADS.ADS1115(i2c)
     ```

4. **Python Implementation** (2h)
   - [ ] Create `battery_monitor.py` module:
     ```python
     class BatteryMonitor:
         VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2  # e.g., 11
         CELLS = 4  # 4S LiPo
         MIN_V = 14.0  # 3.5V per cell
         WARN_V = 15.0  # 3.75V per cell
         MAX_V = 16.8  # 4.2V per cell

         def read_voltage(self):
             raw = ads.read_adc(0, gain=2/3)  # ±6.144V range
             voltage = raw * (6.144 / 32767) * self.VOLTAGE_DIVIDER_RATIO
             return voltage

         def get_soc(self, voltage):
             # Rough SOC estimation from voltage
             # Return 0-100%
             pass

         def check_status(self):
             v = self.read_voltage()
             if v < self.MIN_V:
                 return "CRITICAL_LOW"
             elif v < self.WARN_V:
                 return "LOW"
             return "OK"
     ```

5. **Integration with Flask** (1h)
   - [ ] Add `/battery` endpoint:
     ```json
     {"voltage": 46.2, "soc": 85, "status": "OK"}
     ```
   - [ ] Display in web UI (battery icon, voltage)
   - [ ] Auto-stop if voltage critical:
     ```python
     if battery_monitor.check_status() == "CRITICAL_LOW":
         send_command('s')
         app.logger.warning("Low battery - stopping")
     ```

6. **Logging** (30min)
   - [ ] Log voltage to CSV every 10 seconds
   - [ ] Include in telemetry history

**Success Criteria:**
- [ ] Voltage reading accurate within ±0.5V (calibrate with multimeter)
- [ ] Web UI shows live voltage
- [ ] System stops automatically at low voltage threshold
- [ ] No false triggers during motor start/stop

---

## 🎯 Milestone 2: Core Usability Upgrades (Weeks 3-4)

### M2.1: Wireless UART Bridge (ESP32)

**Status:** Not started
**Dependencies:** Milestone 1 recommended
**Risk:** Medium (networking complexity)

#### Architecture Options:

**Option A: ESP32 WiFi Serial Bridge (Recommended)**
- Both RPi and Teensy have ESP32
- ESP32s create WiFi access point or connect to existing network
- UDP or TCP socket connection replaces UART cable
- Range: ~50m indoor, ~100m outdoor

**Option B: XBee Series 2**
- Dedicated RF modules
- Longer range (300m+)
- Lower bandwidth, but simpler (transparent UART)

**Option C: Bluetooth SPP**
- Built-in to RPi and ESP32
- Shorter range (10-30m)
- More latency

#### Tasks (Option A - ESP32):
1. **Procurement** (1h)
   - [ ] ESP32 Dev Board ×2
   - [ ] Logic level shifters (if needed, ESP32 is 3.3V)

2. **ESP32 Firmware** (4h)
   - [ ] Program both ESP32s with WiFiSerial bridge code
     - Station mode on RPi ESP32
     - AP mode on Teensy ESP32 (or both station mode)
   - [ ] Implement reconnection logic
   - [ ] Add heartbeat/health monitoring
   - [ ] Configure baud rate matching (115200)

   Example code structure:
   ```cpp
   // ESP32 WiFi Serial Bridge
   #include <WiFi.h>
   #include <WiFiServer.h>
   #include <HardwareSerial.h>

   HardwareSerial Serial(2);  // UART2 for Teensy connection
   WiFiServer server(8080);

   void setup() {
     Serial.begin(115200);  // To Teensy
     WiFi.begin(ssid, password);
     server.begin();
   }

   void loop() {
     WiFiClient client = server.available();
     if (client) {
       // Bridge data between Serial and WiFi client
       while (client.connected()) {
         if (Serial.available()) {
           client.write(Serial.read());
         }
         if (client.available()) {
           Serial.write(client.read());
         }
       }
     }
   }
   ```

3. **RPi Python Modifications** (2h)
   - [ ] Create `serial_wifi.py` module:
     ```python
     import socket
     class WiFiSerial:
         def __init__(self, host, port=8080):
             self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
             self.sock.connect((host, port))

         def write(self, data):
             self.sock.send(data)

         def read(self, size=1):
             return self.sock.recv(size)
     ```
   - [ ] Modify `pythonfile.py` to use WiFiSerial when `/dev/serial0` not available
   - [ ] Add fallback: try WiFi first, then UART
   - [ ] Configuration flag in `config.yaml`: `comm: wifi` or `comm: uart`

4. **Integration Testing** (2h)
   - [ ] Verify range: 10m, 25m, 50m through walls
   - [ ] Test reconnection after WiFi dropout
   - [ ] Measure latency (should be <50ms)
   - [ ] Test command reliability (send 1000 commands, count errors)

5. **Documentation** (1h)
   - [ ] Create `docs/wifi-bridge.md` with ESP32 flashing instructions
   - [ ] Update HARDWARE.md with WiFi bridge wiring
   - [ ] Update SETUP.md with wireless configuration

**Success Criteria:**
- [ ] Reliable communication up to 50m line-of-sight
- [ ] Reconnects automatically after WiFi interruption
- [ ] Latency <100ms RTT
- [ ] Command loss rate <0.1%

---

### M2.2: Wheel Encoders for Odometry

**Status:** Not started
**Dependencies:** None (but integrates with PID control later)
**Risk:** Low

#### Hardware Choice:
- **Option A:** Hall effect sensors + magnets (cheap, ~$5/motor)
- **Option B:** Optical encoders (more precise, ~$15-30/motor)
- **Resolution:** 500-2000 counts per revolution

#### Tasks:
1. **Procurement** (1h)
   - [ ] Encoders ×4 (Hall effect recommended)
   - [ ] Magnets ×4 (if Hall effect) - 6mm diameter ×3mm thick
   - [ ] Mounting hardware (3D print or aluminum brackets)

2. **Mechanical Installation** (2h)
   - [ ] Mount magnet to motor shaft (may need coupler/adapter)
   - [ ] Mount Hall sensor ~3mm from magnet
   - [ ] Secure wiring with cable ties
   - [ ] Test for signal with oscilloscope/multimeter while spinning

3. **Teensy Integration** (3h)
   - [ ] Connect encoder outputs to Teensy pins (use interrupt-capable pins: 0-23)
     ```cpp
     // FL: GPIO10, FR: GPIO11, RL: GPIO12, RR: GPIO13 (example)
     Encoder encFL(10, 11);  // A, B
     Encoder encFR(12, 13);
     // ...
     ```
   - [ ] Install Encoder library: `Sketch → Include Library → Manage Libraries → Encoder`
   - [ ] Test encoder counts:
     ```cpp
     void loop() {
       Serial.print("FL: "); Serial.println(encFL.read());
       delay(100);
     }
     ```
   - [ ] Calibrate: rotate wheel by hand, verify counts match rotations

4. **Odometry Calculation** (2h)
   - [ ] Compute wheel circumference: `C = π × wheel_diameter_m`
   - [ ] Compute meters per count: `mpc = C / counts_per_rev`
   - [ ] Track position in Teensy:
     ```cpp
     // In loop() at ~100Hz
     long left_counts = encFL.read() + encRL.read();
     long right_counts = encFR.read() + encRR.read();

     float left_distance = left_counts * mpc;
     float right_distance = right_counts * mpc;

     float linear_velocity = (left_distance + right_distance) / 2.0 / dt;
     float angular_velocity = (right_distance - left_distance) / wheelbase / dt;

     // Send to RPi
     Serial.printf("O%.3f,%.3f,%.3f\n", linear_velocity, angular_velocity, heading);
     ```

5. **RPi Integration** (2h)
   - [ ] Parse odometry messages from Teensy (`O<v>,<ω>,<θ>\n`)
   - [ ] Fuse with MPU6050 heading for dead reckoning
   - [ ] Create `/odom` topic (for ROS2 later) or simple state object
   - [ ] Store pose history (x, y, θ) in `state['pose']`

6. **Web UI Display** (1h)
   - [ ] Add panel showing:
     - Current pose (x, y, heading)
     - Wheel speeds (m/s)
     - Encoder counts per wheel
   - [ ] Add button to reset pose to (0,0,0)

**Success Criteria:**
- [ ] Encoder counts accurately reflect wheel rotations (<1% error)
- [ ] Dead reckoning position accurate to <5% over 10m straight line
- [ ] Heading from gyro + wheel spin integrated

---

## 🎯 Milestone 3: Advanced Control (Weeks 5-7)

### M3.1: PID Speed Control

**Status:** Not started
**Dependencies:** M2.2 (Wheel Encoders)
**Risk:** Medium (tuning required)

#### Architecture:
- Teensy runs PID loop at ~100-1000Hz
- Setpoint comes from RPi (desired velocity in m/s or throttle %)
- Feedback from wheel encoders
- Output: PWM duty cycle (0-255)

#### Tasks:
1. **Add PID Library** (30min)
   ```cpp
   #include <PID_v1.h>
   // Per-side PID controllers
   PID leftPID(&leftVelocity, &leftOutput, &leftSetpoint, Kp, Ki, Kd, DIRECT);
   PID rightPID(&rightVelocity, &rightOutput, &rightSetpoint, Kp, Ki, Kd, DIRECT);
   ```

2. **Implement Velocity Feedback** (2h)
   - Convert encoder counts to linear velocity (m/s):
     ```cpp
     float countsToVelocity(long counts, float dt) {
         return counts * METERS_PER_COUNT / dt;
     }
     ```
   - Compute velocity every 10ms (100Hz loop)

3. **New Command: `v<velocity>`** (1h)
   - Add to `moveVehicle()`:
     ```cpp
     case 'v': {
         float target_vel = value / 100.0;  // e.g., v50 → 0.5 m/s
         leftSetpoint = target_vel;
         rightSetpoint = target_vel;
         break;
     }
     ```
   - PID runs automatically in background loop
   - In `loop()`, update motor throttles from `leftOutput`, `rightOutput`

4. **Tuning Interface** (2h)
   - Expose PID gains via commands:
     - `p<Kp>` - set proportional gain
     - `i<Ki>` - set integral gain
     - `d<Kd>` - set derivative gain
   - Store in EEPROM or Teensy `Preferences`
   - Web UI sliders to adjust live

5. **Python Integration** (2h)
   - Replace `d<correction>` with velocity-based:
     ```python
     # For straight driving:
     send_command("v50")  # 0.5 m/s

     # For turning with speed control:
     # Send differential velocity:
     # v<left_speed>,<right_speed>
     # e.g., "v40,60" → left 0.4 m/s, right 0.6 m/s (gentle right turn)
     ```
   - Update `driveContinuous()` to send velocity commands

6. **Calibration & Tuning** (3h)
   - Start with Kp only: `Kp=0.1`, `Ki=0`, `Kd=0`
   - Test step response: command 0 → 0.5 m/s, observe overshoot/rise time
   - Increase Kp until sluggish → oscillating, then back off 30%
   - Add Ki to eliminate steady-state error (use cautiously to avoid windup)
   - Add Kd to damp oscillations
   - Document optimal gains for different surfaces

7. **Safety Limits** (1h)
   - Max velocity limit (e.g., 2.0 m/s)
   - Min velocity (0.05 m/s to prevent stall)
   - Current sensing (if available) → torque limit
   - Watchdog: if no command for 1s, stop

**Success Criteria:**
- [ ] Speed maintained within ±5% of setpoint on flat surface
- [ ] <0.5s response time to speed changes
- [ ] No integral windup causing overshoot
- [ ] Smooth acceleration (no jerky starts)

---

### M3.2: Pure Pursuit Path Following

**Status:** Not started
**Dependencies:** M3.1 (PID control + odometry)
**Risk:** Medium (algorithm complexity)

#### Tasks:
1. **Research & Algorithm Selection** (2h)
   - Pure Pursuit: simple, circular lookahead
   - Stanley Controller: used in Stanford AV
   - Choose Pure Pursuit for simplicity

2. **Implement Path Following** (4h)
   ```python
   class PurePursuit:
       def __init__(self, lookahead=0.5, wheelbase=0.6):
           self.lookahead = lookahead  # meters
           self.wheelbase = wheelbase  # meters between wheel axles

       def compute_steering(self, pose, path):
           """
           pose: (x, y, yaw)
           path: list of (x, y) waypoints
           Returns: curvature (-1 to 1, normalized)
           """
           # Find lookahead point on path
           target = self.find_lookahead_point(pose[:2], path)
           if target is None:
               return 0

           # Transform target to vehicle frame
           dx = target[0] - pose[0]
           dy = target[1] - pose[1]
           local_x = dx * cos(-pose[2]) - dy * sin(-pose[2])
           local_y = dx * sin(-pose[2]) + dy * cos(-pose[2])

           # Pure pursuit geometry
           curvature = 2 * local_y / (self.lookahead ** 2)
           return np.clip(curvature, -1, 1)
   ```

3. **Waypoint Editor** (4h)
   - [ ] Create web UI map interface:
     - Click to add waypoints
     - Drag to reorder
     - Import/export JSON
   - [ ] Store path in RPi memory or file
   - [ ] Visualize path overlayed on camera (or map)

4. **Controller Loop** (2h)
   ```python
   class PathFollower:
       def __init__(self):
           self.path = []
           self.pure_pursuit = PurePursuit(lookahead=0.5)
           self.state = "IDLE"

       def follow_path(self):
           while self.state == "FOLLOWING":
               pose = get_current_pose()  # from odometry
               curvature = self.pure_pursuit.compute_steering(pose, self.path)

               # Convert curvature to wheel speeds
               base_speed = 0.5  # m/s
               delta = curvature * base_speed * self.wheelbase / 2.0
               left_vel = base_speed - delta
               right_vel = base_speed + delta

               # Send to Teensy
               send_command(f"v{left_vel*100:.0f},{right_vel*100:.0f}")

               # Check if reached end of path
               if self.at_goal(pose):
                   self.state = "IDLE"
                   send_command("s")

               time.sleep(0.05)  # 20Hz control loop
   ```

5. **Error Handling** (2h)
   - [ ] Stop if deviation > threshold (path lost)
   - [ ] Recovery behavior: stop, request new path
   - [ ] Timeout: if stuck >30s, abort

6. **Web UI Integration** (2h)
   - [ ] "Start Path" button
   - [ ] Waypoint visualization (overlay on camera or separate plot)
   - [ ] Progress indicator (percentage complete)
   - [ ] Manual override at any time (web buttons take priority)

**Success Criteria:**
- [ ] Follows 10-point path within ±10cm
- [ ] Smooth turns, minimal oscillation
- [ ] Handles sharp corners (adjust lookahead)
- [ ] Recovery from obstruction (pause then resume)

---

## 🎯 Milestone 4: SLAM & Mapping (Month 3)

### M4.1: RPLidar A1 Integration

**Status:** Not started
**Dependencies:** None (parallel to M3)
**Risk:** High (computationally intensive)

#### Tasks:
1. **Procurement** (1h)
   - [ ] RPLidar A1 or A2 (360°, 5-10m range, 5-10Hz)
   - [ ] USB cable (if not included)
   - [ ] Mounting bracket for AGV

2. **Installation & Testing** (2h)
   ```bash
   # Connect RPLidar to USB
   ls /dev/ttyUSB*  # Should detect

   # Install RPLidar Python library
   pip3 install rplidar-roboticia

   # Test scanning
   python3 -m rplidar
   ```

3. **Data Pipeline** (4h)
   - [ ] Create `lidar.py` module:
     ```python
     from rplidar import RPLidar
     class Lidar:
         def __init__(self, port='/dev/ttyUSB0'):
             self.lidar = RPLidar(port)
             self.scan_data = []

         def get_scan(self):
             # Returns list of (angle, distance) tuples
             return list(self.lidar.iter_scans())

         def get_pointcloud(self):
             # Convert polar to Cartesian
             points = []
             for angle, distance in self.scan_data:
                 x = distance * cos(radians(angle))
                 y = distance * sin(radians(angle))
                 points.append((x, y))
             return points
     ```
   - [ ] Run in separate thread (similar to ultrasonic_loop)

4. **Mapping with SLAM** (8h)
   - [ ] Choose SLAM library:
     - **Option A:** `slam_toolbox` (ROS2, needs ROS2 installed - see M5)
     - **Option B:** `python_breeze` (pure Python, simpler but less accurate)
     - **Option C:** `g2o` + custom implementation (most work)
   - **Recommended:** Install ROS2 Humble on RPi (if powerful enough) or on separate laptop + network sync

   ROS2 SLAM Setup (if chosen):
   ```bash
   # On RPi (may be slow) or laptop
   sudo apt install ros-humble-slam-toolbox
   ros2 launch slam_toolbox online_async_launch.py
   ```

   - [ ] Publish `/scan` topic from Lidar
   - [ ] Subscribe to `/map` topic
   - [ ] Save map to file (PGM + YAML)

5. **Integration with Existing System** (2h)
   - [ ] Display map in web UI (as another video stream or static image)
   - [ ] Update map in real-time as AGV moves
   - [ ] Save map to `maps/` directory

6. **Localization** (3h)
   - [ ] Publish `/amcl_pose` (AMCL = Adaptive Monte Carlo Localization)
   - [ ] Subscribe to `/cmd_vel` from Nav2 (later)
   - [ ] Fuse odometry + lidar for accurate pose

**Success Criteria:**
- [ ] Map matches physical environment (walls, obstacles)
- [ ] AGV knows its position within ±10cm
- [ ] Map saving and loading works
- [ ] <5% CPU on RPi 5 (if running on RPi) or network latency acceptable

---

## 🎯 Milestone 5: ROS2 Integration (Months 4-6)

### M5.1: ROS2 Humble Installation

**Status:** Not started
**Dependencies:** None (can do in parallel)
**Risk:** High (steep learning curve)

#### Tasks:
1. **Install ROS2 Humble on RPi 5** (2h)
   ```bash
   sudo apt update && sudo apt install -y curl gnupg lsb-release
   sudo curl -sSL http://repo.ros2.org/repos.key | sudo apt-key add -
   sudo sh -c 'echo "deb [arch=$(dpkg --print-architecture)] http://repo.ros2.org/ubuntu/main $(source /etc/os-release && echo $VERSION_CODENAME) main" > /etc/apt/sources.list.d/ros2.list'
   sudo apt update
   sudo apt install -y ros-humble-ros-base
   ```

   **Note:** Full desktop install may be heavy for RPi; consider:
   - Running ROS2 on separate laptop/desktop, RPi as serial bridge
   - Using micro-ROS on Teensy

2. **ROS2 Workspace Setup** (1h)
   ```bash
   mkdir -p ~/agv_ws/src
   cd ~/agv_ws
   colcon build
   source install/setup.bash
   ```

3. **Create ROS2 Package for AGV** (4h)
   - [ ] `agv_msgs` - Custom message definitions:
     ```python
     # MotorCommand.msg
     float32 left_speed
     float32 right_speed
     float32 left_trim
     float32 right_trim
     ```
   - [ ] `agv_controller` - Node that bridges serial to ROS2
   - [ ] `agv_sensors` - Publishes IMU, odometry, battery
   - [ ] `agv_web_bridge` - Flask <→ ROS2 topic translation

4. **micro-ROS on Teensy (Optional but Cool)** (16h)
   - [ ] Install PlatformIO
   - [ ] Build Teensy firmware as micro-ROS agent
   - [ ] Publish `/wheel_velocities`, `/imu`
   - [ ] Subscribe to `/cmd_vel` from RPi ROS2
   - [ ] Use XRCE-DDS for transport (serial or WiFi)

5. **Nav2 Integration** (8h)
   - [ ] Install Nav2 stack
   - [ ] Configure costmaps (laser, obstacle layer)
   - [ ] Set up behavior tree for navigation
   - [ ] Provide map from SLAM

6. **Testing** (4h)
   - [ ] `ros2 topic list` shows all AGV topics
   - [ ] `ros2 topic echo /odom` works
   - [ ] Teleop: `ros2 topic pub --once /cmd_vel geometry_msgs/Twist ...`
   - [ ] Send navigation goal: `ros2 service call /navigate_to_pose ...`

**Success Criteria:**
- [ ] All sensors publishing ROS2 topics
- [ ] Motor control subscribes to `/cmd_vel`
- [ ] Can use RViz2 to visualize:
  - Laser scan
  - Map
  - Pose estimate
  - Navigation plan
- [ ] Nav2 successfully navigates to goal

---

## 🎯 Milestone 6: Polish & Documentation (Throughout)

### Documentation Tasks:
- [ ] Add code comments with API documentation
- [ ] Create API reference (Doxygen for Teensy, Sphinx for Python)
- [ ] Record video demonstrations
- [ ] Build detailed BOM spreadsheet with suppliers and prices
- [ ] Create assembly guide with photos (if building multiple)

### Code Quality:
- [ ] Add unit tests for Python (pytest)
- [ ] Add integration tests (simulated serial communication)
- [ ] Set up code coverage reporting
- [ ] Add pre-commit hooks (black, flake8, mypy)
- [ ] Add static analysis for C++ (clang-tidy)

### Safety:
- [ ] Add software watchdog (stops motors if heartbeat lost)
- [ ] Add emergency stop API (hardware + software)
- [ ] Implement soft limits (max speed, acceleration)
- [ ] Add battery cutoff relay controlled by Teensy

### Performance:
- [ ] Profile Python code, optimize bottlenecks
- [ ] Move Flask to separate thread or use FastAPI for async
- [ ] Reduce camera resolution if bandwidth limited
- [ ] Implement frame skipping if CPU overloaded

---

## 📅 Timeline Summary

| Week | Milestone | Tasks |
|------|-----------|-------|
| 1-2  | M1: Safety & Power | E-stop, Battery monitor |
| 2-3  | M2.1: Wireless Bridge | ESP32 WiFi serial |
| 3-4  | M2.2: Encoders | Odometry |
| 4-6  | M3.1: PID Control | Speed regulation |
| 6-7  | M3.2: Pure Pursuit | Path following |
| 7-9  | M4: SLAM | RPLidar mapping |
| 10+  | M5: ROS2 | Full ecosystem integration |

---

## 🗂️ Branch Strategy

```
main (stable releases)
  ├── develop (integration branch)
  │    ├── feature/wheel-encoders
  │    ├── feature/pid-control
  │    ├── feature/wireless-bridge
  │    ├── feature/slam-lidar
  │    └── feature/ros2-integration
  │
  ├── hotfix/issue-XXX (branched from main)
  │
  └── release/v1.1.0 (tagged releases)
```

**Workflow:**
1. `git checkout develop`
2. `git checkout -b feature/<name>`
3. Implement, test, commit
4. `git push origin feature/<name>`
5. Open PR to `develop`
6. After testing & CI, merge to `develop`
7. Periodically merge `develop` → `main` with version tag

---

## ✅ Completion Checklist

Before marking a milestone complete:
- [ ] All tasks in milestone finished
- [ ] Code reviewed (self-review + peer if available)
- [ ] Documentation updated
- [ ] Tests passing
- [ ] CI pipeline green
- [ ] Hardware tested in field (not just bench)
- [ ] Video demonstration recorded
- [ ] Lessons learned documented

---

## 🔄 Iteration Cycle

Each milestone follows:
1. **Plan:** Review tasks, order dependencies
2. **Implement:** Code, wire, test
3. **Document:** Update docs, write notes
4. **Demo:** Record video showing feature working
5. **Retrospect:** What went well, what to improve
6. **Release:** Merge to main, tag version if significant

---

## 📞 Communication

- **Updates:** Push commits frequently to feature branch
- **Issues:** Open GitHub issue for blockers
- **Questions:** Use GitHub Discussions
- **Progress:** Update this plan as you go (check boxes)

---

**Ready to build!** Start with **Milestone 1.1** - E-Stop (safety first).

🚀 Let's make this AGV awesome!
