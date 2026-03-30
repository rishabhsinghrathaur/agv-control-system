# Contributing to AGV Control System

Thank you for your interest in improving this project! This guide will help you get started.

---

## How to Contribute

### 1. Find an Issue or Create One

- Browse existing [issues](https://github.com/rishabhsinghrathaur/agv-control-system/issues) for bugs or feature requests
- Or create a new issue describing the problem or enhancement

### 2. Fork the Repository

Click the "Fork" button on GitHub to create your own copy.

### 3. Clone Your Fork

```bash
git clone https://github.com/YOUR_USERNAME/agv-control-system.git
cd agv-control-system
```

### 4. Create a Feature Branch

```bash
git checkout -b feature/amazing-feature
# or for a bugfix:
git checkout -b fix/issue-#123
```

**Branch naming conventions:**
- `feature/<description>` - New feature
- `fix/<description>` - Bug fix
- `docs/<description>` - Documentation update
- `chore/<description>` - Build/process changes

### 5. Make Your Changes

- Follow existing code style
- Add tests if applicable
- Update documentation (README, docs/, comments)
- Commit with clear, descriptive messages

```bash
git add .
git commit -m "feat: add battery voltage monitoring"
```

**Commitmessage format (optional but recommended):**
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation
- `style:` - Formatting (no code change)
- `refactor:` - Code restructuring
- `test:` - Adding/updating tests
- `chore:` - Build/process changes

### 6. Push to Your Fork

```bash
git push origin feature/amazing-feature
```

### 7. Open a Pull Request

- Go to the original repository on GitHub
- Click "Compare & pull request"
- Select your branch
- Fill out the PR template:
  - Describe the changes
  - Reference related issue (e.g., "Closes #123")
  - Add screenshots if UI changes
  - Note any breaking changes or migration steps

### 8. Code Review

- Maintainers will review your PR
- Address requested changes
- Once approved, your PR will be merged

---

## Development Guidelines

### Code Style

**Python:**
- Follow PEP 8
- Use 4 spaces for indentation
- Max line length: 88 characters (Black formatter default)
- Type hints encouraged
- Docstrings for public functions/classes

Example:
```python
def send_command(cmd: str) -> bool:
    """Send command to Teensy via UART.

    Args:
        cmd: Command string (e.g., "d20", "s")

    Returns:
        True if command sent successfully, False otherwise
    """
    if ser and ser.is_open:
        ser.write((cmd + "\n").encode())
        return True
    return False
```

**Arduino/C++:**
- K&R braces style (opening brace on same line)
- 2 spaces for indentation (Arduino style)
- Upper camel case for class names, lowercase with underscores for functions/variables
- Comment complex logic

### Testing

- Test hardware changes with actual components when possible
- For sensor changes, validate readings against known values
- For motor control, test at low speed first with safety measures

### Documentation

- Update README.md for user-facing changes
- Update docs/ for technical details (protocol, troubleshooting, upgrades)
- Add docstrings/comments for non-obvious code
- Include pin numbers, wiring details for hardware changes

### Commit Sensibly

- One logical change per commit
- Don't commit debug prints or temporary files
- Squash trivial fixes before merging

---

## Areas for Contribution

### High Priority
- **Bug fixes** - Check issues labeled `bug`
- **Documentation improvements** - Clarify confusing sections, add examples
- **Hardware compatibility** - Test with different BLDC controllers, IMUs, etc.
- **Safety features** - E-stop implementation, battery monitoring

### Medium Priority
- **PID speed control** - Implement closed-loop velocity control
- **Wheel encoder support** - Add odometry
- **Wireless communication** - XBee/ESP32 serial bridge
- **ROS2 integration** - micro-ROS on Teensy, ROS2 nodes on RPi

### Low Priority / Experimental
- **SLAM** - RPLidar integration
- **Multi-AGV coordination** - Fleet management
- **Deep learning navigation** - TensorFlow Lite deployment
- **Advanced sensors** - Depth camera, RTK GPS

---

## Getting Help

- **Questions?** Open an issue with `question` label
- **Stuck?** Check `docs/troubleshooting.md`
- **Hardware issues** - Post wiring diagrams/photos
- **Discussion?** Use GitHub Discussions (if enabled)

---

## Code of Conduct

- Be respectful and inclusive
- Constructive criticism only
- No harassment or discriminatory language
- Focus on what's best for the community

See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for full text.

---

## License

By contributing, you agree that your contributions will be licensed under the MIT License of the project.

---

## Recognition

Contributors will be acknowledged in:
- README.md Contributors section
- Release notes
- Project website (if created)

---

Thank you for making this AGV platform better for everyone!

🚀 Happy hacking!
