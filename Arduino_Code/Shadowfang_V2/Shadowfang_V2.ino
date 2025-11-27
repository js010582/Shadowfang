#include <Mouse.h>

const int BUTTON_PIN = 6;   // Button to GND (INPUT_PULLUP)
const int LED_PIN    = 9;  // External LED (sinking: LOW=ON, HIGH=OFF)

const unsigned long DEBOUNCE_MS   = 35;
const unsigned long MOVE_INTERVAL = 3;      // ms between micro-steps
const unsigned long LOOP_INTERVAL = 10000;  // ms idle between bursts

// --- Button debouncer (edge-based) ---
int rawBtn = HIGH, lastRawBtn = HIGH, stableBtn = HIGH; // HIGH = not pressed
unsigned long lastRawChangeMs = 0;
bool btnFell = false, btnRose = false;

// --- State machine ---
enum State { IDLE, MOVING, BETWEEN };
State state = IDLE;
bool active = false;

// --- Movement burst bookkeeping ---
int stepsRemaining = 0;
int stepX = 0, stepY = 0;
unsigned long lastStepMs = 0;
unsigned long betweenStartMs = 0;

void startBurst(unsigned long now) {
  int distance = random(10, 800);
  stepX = (random(3) - 1);         // -1, 0, or +1
  stepY = (random(3) - 1);         // -1, 0, or +1
  if (stepX == 0 && stepY == 0) {  // ensure movement
    stepX = 1;
  }
  stepsRemaining = distance;
  lastStepMs = now;
  state = MOVING;
}

void stopActive() {
  active = false;
  state = IDLE;
  Serial.println("Mouse mover OFF");
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // sinking LED off

  Serial.begin(9600);
  randomSeed(analogRead(0));
  Mouse.begin();
}

void loop() {
  unsigned long now = millis();

  // ---- Debounce (detect edges once) ----
  rawBtn = digitalRead(BUTTON_PIN);
  if (rawBtn != lastRawBtn) {
    lastRawBtn = rawBtn;
    lastRawChangeMs = now;
  }
  if ((now - lastRawChangeMs) >= DEBOUNCE_MS && rawBtn != stableBtn) {
    stableBtn = rawBtn;
    btnFell = (stableBtn == LOW);   // pressed
    btnRose = (stableBtn == HIGH);  // released
  }

  // ---- Handle button press (toggle active) ----
  if (btnFell) {
    active = !active;
    Serial.println(active ? "Mouse mover ON" : "Mouse mover OFF");
    if (active) {
      startBurst(now);
    } else {
      state = IDLE;
    }
    btnFell = false; // consume edge
  }
  if (btnRose) btnRose = false; // not needed further, just clear

  // ---- LED reflects active state ----
  digitalWrite(LED_PIN, active ? LOW : HIGH);  // LOW = ON (sinking)

  // ---- State machine ----
  switch (state) {
    case IDLE:
      // Do nothing unless activated
      break;

    case MOVING:
      // Allow immediate stop mid-move
      if (!active) { state = IDLE; break; }
      if (stepsRemaining > 0 && (now - lastStepMs) >= MOVE_INTERVAL) {
        Mouse.move(stepX, stepY, 0);
        lastStepMs = now;
        stepsRemaining--;
      }
      if (stepsRemaining <= 0) {
        betweenStartMs = now;
        state = BETWEEN;
      }
      // If user pressed during moving, the toggle above already handled it
      if (!active) { state = IDLE; }
      break;

    case BETWEEN:
      if (!active) { state = IDLE; break; }
      if ((now - betweenStartMs) >= LOOP_INTERVAL) {
        startBurst(now);
      }
      break;
  }
}
