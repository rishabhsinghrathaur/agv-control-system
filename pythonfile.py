[3/30/26 12:31 AM] Rishabh: from flask import Flask, Response, render_template_string, jsonify
import cv2
import serial
import time
import threading
import RPi.GPIO as GPIO
from mpu6050 import mpu6050
import math

# ================= CONFIG =================
SERIAL_PORT = '/dev/serial0'
BAUD_RATE = 115200

TURN_TARGET = 15
TURN_SPEED_ADJUST = 0.9

TRIG_F, ECHO_F = 23, 24
TRIG_B, ECHO_B = 17, 27

OBSTACLE_DIST = 70

# ================= STATE =================
user_direction = "STOP"
obs_front = False
obs_back = False
tag_obs_front = False

last_command = None

# ================= HARDWARE =================
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2)
    print("✅ UART Connected")
except:
    ser = None

# MPU
try:
    sensor = mpu6050(0x68)
    print("✅ MPU Connected")
except:
    sensor = None

# GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIG_F, GPIO.OUT)
GPIO.setup(ECHO_F, GPIO.IN)
GPIO.setup(TRIG_B, GPIO.OUT)
GPIO.setup(ECHO_B, GPIO.IN)

# Camera
cap = cv2.VideoCapture(0)
cap.set(3, 320)
cap.set(4, 240)

# ================= SAFE SEND =================
def send_command(cmd):
    global last_command
    if cmd != last_command:
        if ser:
            ser.write((cmd + "\n").encode())
            print(f"📡 {cmd}")
        last_command = cmd

# ================= ULTRASONIC =================
def get_distance(trig, echo):
    GPIO.output(trig, True)
    time.sleep(0.00001)
    GPIO.output(trig, False)

    start = time.time()
    end = time.time()
    timeout = time.time() + 0.05

    while GPIO.input(echo) == 0 and time.time() < timeout:
        start = time.time()

    while GPIO.input(echo) == 1 and time.time() < timeout:
        end = time.time()

    return (end - start) * 17150

def ultrasonic_loop():
    global obs_front, obs_back

    stable_f = 0
    stable_b = 0

    while True:
        d_f = get_distance(TRIG_F, ECHO_F)
        stable_f = stable_f + 1 if d_f < OBSTACLE_DIST else 0
        obs_front = stable_f >= 2

        time.sleep(0.05)

        d_b = get_distance(TRIG_B, ECHO_B)
        stable_b = stable_b + 1 if d_b < OBSTACLE_DIST else 0
        obs_back = stable_b >= 2

        if user_direction == "f":
            send_command("s" if (obs_front or tag_obs_front) else "f")

        elif user_direction == "b":
            send_command("s" if obs_back else "b")

        else:
            send_command("s")

        time.sleep(0.1)

threading.Thread(target=ultrasonic_loop, daemon=True).start()

# ================= MPU TURN =================
gyro_bias = 0

def calibrate_gyro():
    global gyro_bias
    total = 0
    for _ in range(100):
        total += sensor.get_gyro_data()['z']
        time.sleep(0.01)
    gyro_bias = total / 100

if sensor:
    calibrate_gyro()

def execute_turn(direction):
    global user_direction

    if not ser or not sensor:
        return

    user_direction = "STOP"

    angle = 0
    last = time.time()
    sign = 1 if direction == 'l' else -1

    send_command(direction)

    start = time.time()

    while abs(angle) < TURN_TARGET * TURN_SPEED_ADJUST:
        if time.time() - start > 3:
            break

        now = time.time()
        dt = now - last
        last = now

        z = sensor.get_gyro_data()['z'] - gyro_bias
        angle += sign * z * dt

        time.sleep(0.01)

    send_command("s")

# ================= FLASK =================
app = Flask(name)

HTML_UI = """
<!DOCTYPE html>
<html>
<head>
<title>AGV Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body { background:#1a1a1a; color:white; text-align:center; font-family:Arial; }
h2 { color:#00ffcc; }
img { width:100%; max-width:400px; border-radius:10px; }
.controls {
 display:grid; grid-template-columns:1fr 1fr 1fr;
 gap:15px; max-width:350px; margin:20px auto;
}
button {
 padding:25px; font-size:20px; border:none; border-radius:12px;
 background:#444; color:white;
}
button:active { background:#00ffcc; color:black; }
.btn-stop { background:#ff4444; }
.btn-turn { background:#0088ff; }
.warn { color:red; font-weight:bold; }
</style>
</head>
<body>
[3/30/26 12:31 AM] Rishabh: <h2>AGV CONTROL PANEL</h2>
<img src="/video_feed">

<div class="warn" id="warn_f"></div>
<div class="warn" id="warn_b"></div>

<div class="controls">
<div></div>
<button onmousedown="send('f')" ontouchstart="send('f')">▲</button>
<div></div>

<button class="btn-turn" onclick="send('l')">◀ 15°</button>
<button class="btn-stop" onclick="send('s')">STOP</button>
<button class="btn-turn" onclick="send('r')">15° ▶</button>

<div></div>
<div></div>
<div></div>

<div></div>
<button onmousedown="send('b')" ontouchstart="send('b')">▼</button>
<div></div>
</div>

<script>
function send(cmd){ fetch('/cmd/'+cmd); }

document.addEventListener('mouseup', stopMove);
document.addEventListener('touchend', stopMove);

function stopMove(e){
 if(e.target.innerText==='▲'||e.target.innerText==='▼'){
   send('s');
 }
}

setInterval(()=>{
 fetch('/status').then(r=>r.json()).then(d=>{
   document.getElementById('warn_f').innerText = d.front ? "⚠ FRONT OBSTACLE" : "";
   document.getElementById('warn_b').innerText = d.back ? "⚠ BACK OBSTACLE" : "";
 });
},300);
</script>

</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_UI)

@app.route('/cmd/<cmd>')
def cmd(cmd):
    global user_direction

    if cmd in ['l','r']:
        threading.Thread(target=execute_turn, args=(cmd,), daemon=True).start()

    elif cmd == 'f':
        user_direction = 'f'

    elif cmd == 'b':
        user_direction = 'b'

    elif cmd == 's':
        user_direction = 'STOP'
        send_command("s")

    return "OK"

@app.route('/status')
def status():
    return jsonify({"front": obs_front, "back": obs_back})

def gen_frames():
    while True:
        ret, frame = cap.read()
        if not ret:
            continue
        _, buffer = cv2.imencode('.jpg', frame)
        yield (b'--frame\r\nContent-Type:image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

# ================= MAIN =================
if name == "main":
    app.run(host='0.0.0.0', port=5000, threaded=True)