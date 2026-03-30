# Changelog

All notable changes to the AGV Control System project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added
- Complete project repository structure
- Teensy 4.1 firmware with dual-mode command protocol
- Raspberry Pi 5 Flask application with web UI
- MPU6050 gyroscope integration for precise turning
- HC-SR04 ultrasonic obstacle avoidance
- MJPEG camera streaming
- Comprehensive documentation:
  - README with quick start
  - SETUP.md detailed installation guide
  - HARDWARE.md wiring and pinouts
  - docs/protocol.md communication reference
  - docs/troubleshooting.md problem-solving
  - docs/upgrades.md upgrade catalog
  - schematics/pinout.md quick reference
- GitHub integration:
  - MIT License
  - CI/CD workflow (Python lint, Arduino compile)
  - Contributing guidelines
  - Code of conduct
- Implementation plan (IMPLEMENTATION_PLAN.md) with detailed milestones

### Planned
- **Milestone 1: Safety & Power (Weeks 1-2)**
  - Emergency stop hardware button
  - Battery voltage monitoring with ADS1115
- **Milestone 2: Core Usability (Weeks 3-4)**
  - Wireless UART bridge (ESP32)
  - Wheel encoders for odometry
- **Milestone 3: Advanced Control (Weeks 5-7)**
  - PID speed control on Teensy
  - Pure Pursuit path following on RPi
- **Milestone 4: SLAM & Mapping (Month 3)**
  - RPLidar integration
  - Occupancy grid mapping
- **Milestone 5: ROS2 Integration (Months 4-6)**
  - ROS2 Humble nodes
  - micro-ROS on Teensy
  - Nav2 navigation stack

---

## [1.0.0] - 2026-03-30

### Added
- Initial public release
- Fully functional 4-motor BLDC control system
- Web-based control interface
- Obstacle avoidance with dual ultrasonics
- Gyro-based precise turning (±15° accuracy)
- Real-time camera streaming
- UART communication protocol (115200 baud)
- Complete hardware setup documentation
- Troubleshooting guide with solutions to common issues
- GitHub Pages-ready documentation structure
- CI workflow for code quality checks (flake8, Black, Arduino compile)
- Comprehensive upgrade roadmap with 20+ planned features

### Technical Details
- Teensy 4.1 firmware with 20kHz PWM, 8-bit resolution
- Raspberry Pi 5 software with Flask, OpenCV, serial communication
- MPU6050 IMU with automatic gyro calibration
- Thread-safe sensor handling and obstacle detection
- Dual serial interfaces (USB debug + UART control)
- PID-ready control architecture for future speed control

---

## Version History Format

- **Added** for new features
- **Changed** for changes in existing functionality
- **Deprecated** for soon-to-be removed features
- **Removed** for now removed features
- **Fixed** for any bug fixes
- **Security** in case of vulnerabilities

---

**Note:** This changelog started with version 1.0.0. Earlier development versions were not documented.
