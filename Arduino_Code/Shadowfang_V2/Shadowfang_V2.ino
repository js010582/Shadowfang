#include <Mouse.h>

const int BUTTON_PIN = 8;   // Button to GND
const int LED_PIN    = 10;  // External LED (sinking: LOW=ON, HIGH=OFF)

int move_interval = 3;       // ms between micro-moves
int loop_interval = 10000;   // ms idle between movement cycles
bool active = false;         // mouse mover state

// For debounce / edge detection
bool waitRelease = false;
const unsigned long DEBOUNCE_MS = 35;
unsigned long lastEdgeMs = 0;
int lastReading = HIGH; // INPUT_PULLUP: HIGH = not pressed

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off initially (sinking logic)
  
  Serial.begin(9600);
  randomSeed(analogRead(0));
  Mouse.begin();
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // debounce on level change
  if (reading != lastReading) {
    lastEdgeMs = now;
    lastReading = reading;
  }

  // after stable for DEBOUNCE_MS
  if ((now - lastEdgeMs) > DEBOUNCE_MS) {
    if (!waitRelease && reading == LOW) {   // button just pressed
      active = !active;
      Serial.println(active ? "Mouse mover ON" : "Mouse mover OFF");
      waitRelease = true;                   // ignore until released
    }
    if (waitRelease && reading == HIGH) {   // button released
      waitRelease = false;
    }
  }

  // LED control
  digitalWrite(LED_PIN, active ? LOW : HIGH);  // LOW = ON (sinking)

  // Main mouse mover logic
  if (active) {
    int distance = random(10, 800);
    int x = random(3) - 1;
    int y = random(3) - 1;

    for (int i = 0; i < distance && active; i++) {
      Mouse.move(x, y, 0);
      delay(move_interval);

      // Allow stop mid-move
      if (!waitRelease && digitalRead(BUTTON_PIN) == LOW) {
        delay(DEBOUNCE_MS);
        if (digitalRead(BUTTON_PIN) == LOW) {
          active = false;
          Serial.println("Mouse mover OFF");
          waitRelease = true;
          break;
        }
      }
    }

    // Non-blocking wait between movement bursts
    unsigned long start = millis();
    while (active && (millis() - start < (unsigned long)loop_interval)) {
      if (!waitRelease && digitalRead(BUTTON_PIN) == LOW) {
        delay(DEBOUNCE_MS);
        if (digitalRead(BUTTON_PIN) == LOW) {
          active = false;
          Serial.println("Mouse mover OFF");
          waitRelease = true;
        }
      }
    }
  }
}
