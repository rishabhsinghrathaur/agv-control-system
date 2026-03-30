[3/30/26 12:30 AM] Rishabh: // -------- 4 e-bike BLDC controllers - PID READY --------

// ---------- Throttle pins (Teensy 4.1) ----------
#define THROTTLE_FL 28
#define THROTTLE_FR 29
#define THROTTLE_RL 25
#define THROTTLE_RR 24

// ---------- Brake pins (active LOW) ----------
#define BRAKE_FL 5
#define BRAKE_FR 4
#define BRAKE_RL 2
#define BRAKE_RR 3  

// ---------- Reverse pins ----------
#define REVERSE_FL 8
#define REVERSE_FR 9
#define REVERSE_RL 6
#define REVERSE_RR 7

#define PWM_FREQ 20000
#define PWM_RES  8

// --- SPEED & CALIBRATION SETTINGS ---
#define THROTTLE_STOP 0
#define THROTTLE_RUN  129
#define THROTTLE_TURN 140

// If your bot naturally drifts right, increase this number (e.g., 5 or 10)
// This gives a permanent slight power boost to the right wheels.
#define BASE_TRIM_RIGHT 5 

// ---------------- Helpers ----------------
void setAllThrottles(int value) {
  analogWrite(THROTTLE_FL, value);
  analogWrite(THROTTLE_FR, value);
  analogWrite(THROTTLE_RL, value);
  analogWrite(THROTTLE_RR, value);
}

void brakeON() {
  digitalWrite(BRAKE_FL, LOW);
  digitalWrite(BRAKE_FR, LOW);
  digitalWrite(BRAKE_RL, LOW);
  digitalWrite(BRAKE_RR, LOW);
}

void brakeOFF() {
  digitalWrite(BRAKE_FL, HIGH);
  digitalWrite(BRAKE_FR, HIGH);
  digitalWrite(BRAKE_RL, HIGH);
  digitalWrite(BRAKE_RR, HIGH);
}

// ---------------- Direction Logic ----------------
void setForward() {
  digitalWrite(REVERSE_FL, HIGH);
  digitalWrite(REVERSE_FR, LOW);
  digitalWrite(REVERSE_RL, LOW);  
  digitalWrite(REVERSE_RR, HIGH);
}

void setBackward() {
  digitalWrite(REVERSE_FL, LOW);
  digitalWrite(REVERSE_FR, HIGH);
  digitalWrite(REVERSE_RL, HIGH);
  digitalWrite(REVERSE_RR, LOW);
}

void setRight() { // Skid steer right
  digitalWrite(REVERSE_FL, HIGH);
  digitalWrite(REVERSE_RL, LOW);
  digitalWrite(REVERSE_FR, HIGH);
  digitalWrite(REVERSE_RR, LOW);
}

void setLeft() { // Skid steer left
  digitalWrite(REVERSE_FL, LOW);
  digitalWrite(REVERSE_RL, HIGH);
  digitalWrite(REVERSE_FR, LOW);
  digitalWrite(REVERSE_RR, HIGH);
}

// ---------------- REAL-TIME PID FUNCTIONS (NO DELAYS) ----------------

// Command 'd': Drive continuously with steering correction
// Positive correction steers Right. Negative correction steers Left.
void driveContinuous(int correction) {
  setForward();
  brakeOFF();

  int leftSpeed  = THROTTLE_RUN + correction;
  int rightSpeed = THROTTLE_RUN - correction + BASE_TRIM_RIGHT;

  leftSpeed  = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);

  analogWrite(THROTTLE_FL, leftSpeed);
  analogWrite(THROTTLE_RL, leftSpeed);
  analogWrite(THROTTLE_FR, rightSpeed);
  analogWrite(THROTTLE_RR, rightSpeed);
}

// Command 't': Turn continuously (Skid Steer) based on PID speed
// Positive speed = spin right. Negative speed = spin left.
void turnContinuous(int turnSpeed) {
  brakeOFF();
  
  if (turnSpeed > 0) {
    setRight();
  } else {
    setLeft();
  }

  // Calculate absolute speed, ensuring we hit a minimum speed required to actually move the heavy wheels
  int absoluteSpeed = abs(turnSpeed);
  int finalSpeed = constrain(THROTTLE_STOP + absoluteSpeed, 0, 255); 

  setAllThrottles(finalSpeed);
}

// ---------------- Legacy Execution Functions ----------------
void syncedStart(int speed) {
  setAllThrottles(THROTTLE_STOP); 
  brakeOFF();
  delay(150); 
  setAllThrottles(speed);
}

void syncedStop() {
  setAllThrottles(THROTTLE_STOP);
  delay(100);
  brakeON();
}

void moveVehicle(String input) {
  char cmd = input[0];
  int value = 0;
  
  if (input.length() > 1) {
    value = input.substring(1).toInt(); // This safely handles negative numbers too!
  }

  switch(cmd) {
    // --- NEW REAL-TIME COMMANDS FOR RPi PID ---
    case 'd': // e.g., "d15" or "d-10"
      driveContinuous(value);
      break;

    case 't': // e.g., "t50" or "t-50"
      turnContinuous(value);
      break;

    // --- LEGACY COMMANDS (Still work for manual control) ---
    case 's':
      syncedStop();
      break;

    case 'f':
      syncedStop(); delay(200);
      setForward(); syncedStart(THROTTLE_RUN);
      break;
[3/30/26 12:30 AM] Rishabh: case 'b':
      syncedStop(); delay(200);
      setBackward(); syncedStart(THROTTLE_RUN);
      break;
  }
}

// ---------------- Setup & Loop ----------------
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  
  pinMode(BRAKE_FL, OUTPUT); pinMode(BRAKE_FR, OUTPUT);
  pinMode(BRAKE_RL, OUTPUT); pinMode(BRAKE_RR, OUTPUT);
  pinMode(REVERSE_FL, OUTPUT); pinMode(REVERSE_FR, OUTPUT);
  pinMode(REVERSE_RL, OUTPUT); pinMode(REVERSE_RR, OUTPUT);

  analogWriteResolution(PWM_RES);
  analogWriteFrequency(THROTTLE_FL, PWM_FREQ);
  analogWriteFrequency(THROTTLE_FR, PWM_FREQ);
  analogWriteFrequency(THROTTLE_RL, PWM_FREQ);
  analogWriteFrequency(THROTTLE_RR, PWM_FREQ);

  brakeON();
  setAllThrottles(THROTTLE_STOP);

  Serial.println("--- System Initialized ---");
  Serial.println("PID Commands: d<val> (Drive), t<val> (Turn in place)");
}

void loop() {
  // -------- USB Serial --------
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      moveVehicle(command);
    }
  }

  // -------- UART Serial1 --------
  if (Serial1.available() > 0) {
    String command = Serial1.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      moveVehicle(command);
    }
  }
}