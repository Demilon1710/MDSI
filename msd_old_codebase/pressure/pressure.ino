#include <Wire.h>
#include "Adafruit_MPRLS.h"

// Linear Actuator PWM output pin
#define LAC_PWM_PIN 5

// MPRLS pressure sensor pins
#define RESET_PIN  -1
#define EOC_PIN    -1

// Conversion constant from hPa to mmHg
#define HPA_TO_MMHG 0.75006157584566

// Create pressure sensor object
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

// Target pressures for oscillation steps
int pressureSteps[] = {
  140, 139, 138, 137, 136, 135, 134, 133, 132, 131,
  130, 129, 128, 127, 126, 125, 124, 123, 122, 121,
  120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 
  110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 
  100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 
  90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
  79, 78, 77, 76, 75, 74, 73, 72, 71, 70
};

// PWM values corresponding to each pressure step
int pwmLevels[] = {
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180,180,180,180,180,180,180,180,180,180,
  180
};


// Number of oscillation steps (automatically calculated)
const int numSteps = sizeof(pressureSteps) / sizeof(pressureSteps[0]);


// Connect control circuit to pin 3
const int airPumpPin = 3;

// === State Tracking ===
float baseline_mmHg = 0.0;
bool hasPassed160 = false;
bool oscillationStarted = false;
bool done = false;
int stepIndex = 0;
float latePressure = 0;
float systolic = 0.0;
float diastolic = 0.0;
bool automatic = false;

void setup() {
  Serial.begin(115200);

  // Configure airpump
  pinMode(airPumpPin, OUTPUT);

  // Configure actuator pin
  pinMode(LAC_PWM_PIN, OUTPUT);

  // Fully extend actuator initially
  analogWrite(LAC_PWM_PIN, 255);  
  delay(3000);

  Serial.println("Initializing MPRLS sensor...");

  // Inititalize pressure sensor
  if (!mpr.begin()) {
    Serial.println("Sensor init failed.");
    while (1); // Stop execution if sensor fails
  }

  // Record baseline atmospheric pressure
  delay(1000);
  float pressure_hPa = mpr.readPressure();
  baseline_mmHg = pressure_hPa * HPA_TO_MMHG;

  Serial.print("Baseline Pressure Set (mmHg): ");
  Serial.println(baseline_mmHg, 2);
  Serial.println("Waiting for pressure >160 mmHg to begin...");

  delay(10000);
  latePressure = mpr.readPressure() * HPA_TO_MMHG;
  if(latePressure >= 20 + baseline_mmHg) {
    automatic = true;
    Serial.println("Automatic Monitor Detected");
  }
  else {
    Serial.println("Non-Automatic Monitor Detected");
  }
}

void loop() {
  // Only run test once
  if(done) return;

  // Pump runs until pressure exceeds 160 mmHg if device is not automatic
  if(!hasPassed160) {
    analogWrite(airPumpPin, 175);
  }
  else {
    analogWrite(airPumpPin, 0);
  }
  
  // Read current pressure from sensor
  float current_mmHg = mpr.readPressure() * HPA_TO_MMHG;
  
  // Calculate pressure relative to baseline
  float relative_mmHg = round(current_mmHg - baseline_mmHg);

  Serial.print("Relative Pressure (mmHg): ");
  Serial.println(relative_mmHg);

  // Wait for user to pump to ≥160 mmHg
  if (!hasPassed160 && relative_mmHg >= 160.0) {
    hasPassed160 = true;
    Serial.println("Pressure ≥160 mmHg reached. Waiting for drop to ≤140 mmHg to begin oscillations...");
    return;
  }

  // Wait for pressure to drop to ≤140 mmHg to start
  if (hasPassed160 && !oscillationStarted && relative_mmHg <= 140.0) {
    digitalWrite(airPumpPin, 0); // Turn off the air pump
    oscillationStarted = true;
    Serial.println("Pressure dropped to ≤140 mmHg. Starting oscillations...");
    return;
  }

  // Perform oscillations
  if (oscillationStarted) {
    if(stepIndex < numSteps) {
      float pressureTarget = pressureSteps[stepIndex];

      Serial.print("Pressure Target: ");
      Serial.print(pressureTarget);
      Serial.print(" Current Reading: ");
      Serial.println(relative_mmHg);

      // Allow ±1 mmHg tolerance
      if (fabs(relative_mmHg - pressureTarget) <= 1) {
        analogWrite(airPumpPin, 0);
        int pwm = pwmLevels[stepIndex];

        // Record systolic (first oscillation)
        if (systolic == 0.0) {
          systolic = relative_mmHg;
        }

        // Continuously update diastolic
        diastolic = relative_mmHg;

        Serial.print("Oscillating at ");
        Serial.print(relative_mmHg);
        Serial.print(" mmHg -> PWM Retract: ");
        Serial.println(pwm);

        float before = mpr.readPressure() * HPA_TO_MMHG - baseline_mmHg;
        // Retract actuator
        analogWrite(LAC_PWM_PIN, pwm);
        delay(200);

        // Extend actuator
        analogWrite(LAC_PWM_PIN, 255);
        delay(600);

        float after = mpr.readPressure() * HPA_TO_MMHG - baseline_mmHg;

        analogWrite(airPumpPin, 50);

        delay(500);

        Serial.print(" | Pulse amplitude: ");
        Serial.println(before - after);

        stepIndex++;
        // Only one oscillation per loop
      }
    }
    // Done with all steps or pressure too low
    if (stepIndex >= numSteps || relative_mmHg <= 70.0) {
      done = true;
      analogWrite(airPumpPin, 0);
      Serial.println("Oscillations complete.");
      Serial.print("Systolic (First): ");
      Serial.println(systolic, 1);
      Serial.print("Diastolic (Last): ");
      Serial.println(diastolic, 1);

      // Stop actuator
      analogWrite(LAC_PWM_PIN, 0);
      digitalWrite(airPumpPin, LOW);

      // Reset for next cycle
      stepIndex = 0;
      systolic = 0.0;
      diastolic = 0.0;
      Serial.println("=== READY FOR NEXT CYCLE ===");
      Serial.println("Reselect test to restart");

      hasPassed160 = false;
      oscillationStarted = false;
      

      delay(1000);
    }
  }
  // Small delay for pressure polling stability
  delay(100);
}