# AGV (Automated Guided Vehicle) Control System

A comprehensive open-source autonomous vehicle platform featuring **Teensy 4.1** for real-time motor control and **Raspberry Pi 5** for high-level computation, sensor fusion, and web-based interface.

## System Architecture

```
┌─────────────────┐         UART (115200 baud)        ┌─────────────────┐
│  Raspberry Pi 5 │ ◄──────────────────────────────► │   Teensy 4.1   │
│                 │                                  │                 │
│ • Flask Web UI  │                                  │ • 4× BLDC Ctrl  │
│ • MPU6050 Gyro  │                                  │ • PID Control   │
│ • Ultrasonic x2 │                                  │ • Brake Ctrl    │
│ • Camera Feed   │                                  │ • PWM Gen       │
│ • Obstacle Avoid│                                  │                 │
└─────────────────┘                                  └─────────────────┘
         │                                                    │
         │                                                    │
         ▼                                                    ▼
   ┌──────────┐                                       ┌──────────┐
   │  hub     │                                       │  hub     │
   │  motor FL │                                       │  motor FR │
   └──────────┘                                       └──────────┘
         │                                                    │
         │                                                    │
   └──────────┘                                       └──────────┘
         │                                                    │
         ▼                                                    ▼
   ┌──────────┐                                       ┌──────────┘
   │  hub     │
   │  motor RL │
   └──────────┘
         │
         ▼
   ┌──────────┘
```

## Features

### Motor Control (Teensy 4.1)
- **4× BLDC hub motor control** with independent throttle and direction
- **Real-time PID-ready** architecture for precise speed control
- **Dual serial interfaces** (USB + UART) for redundant communication
- **Skid-steer turning** with configurable trim for drift compensation
- **Hardware brake control** (active LOW)
- **20kHz PWM** at 8-bit resolution
- **Zero-latency command execution** - no blocking delays in PID mode

### High-Level Control (Raspberry Pi 5)
- **Flask-based web interface** accessible from any device
- **Real-time camera streaming** (MJPEG over HTTP)
- **MPU6050 gyroscope integration** for accurate 15° turns
- **Dual ultrasonic sensors** (front & back) for obstacle detection
- **Automatic obstacle avoidance** with immediate stop
- **Thread-safe command execution**
- **Live status monitoring** via JSON API

## Quick Start

### Hardware Setup
1. Connect Teensy 4.1 to Raspberry Pi 5 via UART (TX1/RX1, GPIO14/15)
2. Wire 4 BLDC controllers to Teensy throttle pins (28, 29, 25, 24)
3. Connect brake pins to Teensy (5, 4, 2, 3)
4. Wire direction control pins (8, 9, 6, 7)
5. Connect ultrasonic sensors to RPi GPIO
6. Connect MPU6050 to RPi I2C (SDA/SCL)
7. Connect USB camera to RPi

### Software Installation (Raspberry Pi)

```bash
# Clone repository
git clone <your-repo-url>
cd agv-project

# Install dependencies
sudo apt-get update
sudo apt-get install -y python3-pip libmpi-openmpi-dev
pip3 install -r requirements.txt

# Enable UART on Raspberry Pi
sudo raspi-config
# → Interface Options → Serial Port → Disable console, Enable serial port

# Run the server
python3 pythonfile.py
```

### Upload Teensy Firmware

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install [Teensyduino](https://www.pjrc.com/teensy/td_download.html)
3. Open `teensy41.ino` in Arduino IDE
4. Select **Board: Teensy 4.1**
5. Select **USB Serial**
6. Click **Upload**

### Access Web Interface

Open browser to: `http://<raspberry-pi-ip>:5000`

**Controls:**
- **▲** - Forward (hold)
- **▼** - Backward (hold)
- **◀ 15°** - Rotate left by 15°
- **15° ▶** - Rotate right by 15°
- **STOP** - Immediate stop

## Configuration

### Teensy Calibration (`teensy41.ino`)

```cpp
// Adjust if the vehicle drifts to the right during straight movement
#define BASE_TRIM_RIGHT 5  // Range: 0-20, increase to correct right drift

// Throttle thresholds
#define THROTTLE_STOP 0
#define THROTTLE_RUN  129
#define THROTTLE_TURN 140

// PWM frequency (20kHz recommended for ESCs)
#define PWM_FREQ 20000
#define PWM_RES  8
```

### Raspberry Pi Configuration (`pythonfile.py`)

```python
# Serial communication
SERIAL_PORT = '/dev/serial0'  # UART port
BAUD_RATE = 115200

# Turning parameters
TURN_TARGET = 15              # Gyro target angle in degrees
TURN_SPEED_ADJUST = 0.9       # Multiplier for precise turns (0.85-0.95)

# Obstacle detection
OBSTACLE_DIST = 70            # Stop distance in cm

# Ultrasonic sensor GPIO pins
TRIG_F, ECHO_F = 23, 24       # Front sensor
TRIG_B, ECHO_B = 17, 27       # Back sensor
```

## Communication Protocol

### Teensy ↔ Raspberry Pi (UART)

**Format:** ASCII commands terminated by `\n`

**Drive Command (PID mode):**
- `d<value>` - Continuous drive with speed correction
  - Example: `d15` → drive forward with +15 steering correction (right)
  - Example: `d-10` → drive forward with -10 steering correction (left)
  - Range: -50 to +50

**Turn Command (PID mode):**
- `t<value>` - Continuous turn at specified speed
  - Example: `t50` → turn right at speed 50
  - Example: `t-50` → turn left at speed 50
  - Range: -255 to 255

**Legacy Commands (still supported):**
- `f` - Forward (fixed speed)
- `b` - Backward (fixed speed)
- `s` - Stop + engage brakes

**Response:** Teensy echoes commands to both USB Serial and UART Serial.

## Testing & Calibration

### 1. Motor Calibration
- Ensure BLDC controllers are armed (throttle at minimum during power-on)
- Test each motor individually by sending `f`, `b`, `s` via serial monitor

### 2. Trim Adjustment
- Drive forward and observe if vehicle drifts
- Increase `BASE_TRIM_RIGHT` if drifting right (adds boost to right wheels)
- Decrease if drifting left

### 3. Gyro Calibration
- Keep the vehicle perfectly still during MPU6050 initialization
- The code automatically calibrates on startup (100 samples)

### 4. Turn Precision
- Adjust `TURN_SPEED_ADJUST`:
  - Lower (0.85-0.9) → more precise turns, may undershoot
  - Higher (0.95-1.0) → faster turns, may overshoot

### 5. Ultrasonic Tuning
- Adjust `OBSTACLE_DIST` based on stopping distance
- Test with various obstacles to ensure reliable detection

## Project Structure

```
agv-project/
├── teensy41.ino          # Teensy 4.1 firmware
├── pythonfile.py         # Raspberry Pi main application
├── requirements.txt      # Python dependencies
├── README.md            # This file
├── HARDWARE.md          # Detailed wiring diagrams
├── SETUP.md             # Step-by-step setup guide
├── docs/
│   ├── protocol.md      # Complete command reference
│   ├── troubleshooting  # Common issues and solutions
│   └── upgrades.md      # Future enhancements
└── schematics/
    ├── wiring.pdf       # Complete wiring diagram (to be added)
    └── pinout.md        # Pin assignments reference
```

## Technical Specifications

| Component | Specification |
|-----------|---------------|
| **Microcontroller** | Teensy 4.1 (600 MHz ARM Cortex-M7) |
| **SBC** | Raspberry Pi 5 (Broadcom BCM2712, 2.4GHz quad-core) |
| **IMU** | MPU6050 (3-axis gyro + 3-axis accelerometer) |
| **Motor Type** | BLDC Hub Motors (4×) |
| **Communication** | UART @ 115200 baud |
| **PWM Frequency** | 20 kHz |
| **Web Interface** | Flask @ port 5000 |
| **Camera** | USB Camera (MJPEG streaming) |

## Dependencies

### Python (Raspberry Pi)
```
flask>=2.0.0
opencv-python>=4.5.0
pyserial>=3.5
RPi.GPI0>=0.7.0
mpu6050-raspberrypi>=0.1.1
```

Install with:
```bash
pip3 install -r requirements.txt
```

## License

[Choose your license - MIT recommended for open-source hardware/software]

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Submit a Pull Request with detailed description

## Future Upgrades (Roadmap)

- [ ] **Position estimation** - Wheel encoders + IMU fusion (odometry)
- [ ] **Path following** - Pure Pursuit or Stanley controller
- [ ] **SLAM capabilities** - Lidar integration (RPLidar A1/A2)
- [ ] **ROS2 integration** - ROS 2 Hummingbot nodes
- [ ] **Battery management** - ADC-based voltage monitoring + low-voltage cutoff
- [ ] **LED indicators** - Status LEDs for power, errors, mode
- [ ] **Wireless control** - Remove physical cable dependency (XBee/NRF24L01)
- [ ] **B一定** - Add emergency stop hardware button
- [ ] **Multi-AGV coordination** - Networked fleet management
- [ ] **Autonomous docking** - IR beacon or QR code docking station

## Troubleshooting

See [docs/troubleshooting.md](docs/troubleshooting.md) for common issues and solutions.

## Acknowledgments

- Teensy 4.1 framework based on PJRC.com libraries
- BLDC motor control inspired by modern electric vehicle architectures
- Built for educational and research purposes

---

**Last Updated:** 2026-03-30
**Version:** 1.0.0
**Status:** Functional - Ready for testing and upgrades
