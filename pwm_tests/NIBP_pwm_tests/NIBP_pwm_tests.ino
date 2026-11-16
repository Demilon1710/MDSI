#include <Wire.h>
#include "Adafruit_MPRLS.h"

#define LAC_PWM_PIN 5
#define RESET_PIN  -1
#define EOC_PIN    -1
#define airPumpPin  3

//conversion constant from hPa to mmHg
#define HPA_TO_MMHG 0.75006157584566

//create pressure sensor object
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);
float baseline_mmHg = 0.0;


float readRelativePressure() {
  float pressure_hPa = mpr.readPressure();

  return (pressure_hPa * HPA_TO_MMHG) - baseline_mmHg;
}

void testActuator(int pwm, unsigned long retract_time_ms, float baseline_pressure) {
  while(readRelativePressure() <= baseline_pressure) {
    analogWrite(airPumpPin, 175);
  }
  //ensure the air pump is off
  analogWrite(airPumpPin, 0);

  Serial.print("Baseline pressure set to: ");
  Serial.println(baseline_pressure);

  float before = readRelativePressure();

  //retract actuator
  analogWrite(LAC_PWM_PIN, pwm);

  unsigned long startTime = millis();

  while (millis() - startTime < retract_time_ms) {

    unsigned long elapsed = millis() - startTime;

    float pressure = readRelativePressure();

    Serial.print(elapsed);
    Serial.print(",");
    Serial.println(pressure, 3);

    delay(50);
  }

  //extend actuator
  analogWrite(LAC_PWM_PIN, 255);

  delay(600);

  float after = readRelativePressure();

  //stop actuator
  analogWrite(LAC_PWM_PIN, 0);

  Serial.print("PWM: ");
  Serial.println(pwm);

  Serial.print("Pressure before: ");
  Serial.println(before, 3);

  Serial.print("Pressure after: ");
  Serial.println(after, 3);

  Serial.print("Net pressure change: ");
  Serial.println(after - before, 3);
}

void testAirpump(int pwm, float target_pressure){
  //fully extend lin act
  analogWrite(LAC_PWM_PIN, 255);
  delay(1000);
  float start = readRelativePressure();
  unsigned long startTime = millis();
  analogWrite(airPumpPin, pwm);//turn on pump
  while(readRelativePressure() < target_pressure){
  }
  unsigned long elapsed = millis() - startTime;
  analogWrite(airPumpPin, 0);//turn off pump
  Serial.print("From starting presure: ");
  Serial.print(start);
  Serial.print(" to ");
  Serial.print(target_pressure);
  Serial.print(" in ");
  Serial.print(elapsed);
  Serial.println("ms \n Please reset pressure in tube to test next pwm");

}

void runActTests(){
  //small test range for now to make sure this works
  testActuator(100, 200, 160.0);
  testActuator(125, 200, 160.0);
  testActuator(150, 200, 160.0);
  testActuator(175, 200, 160.0);
  Serial.print("All tests finished");
}

void query_user_airpump_test(){
  Serial.println("Enter Desired PWM for airpump: ");
  while (Serial.available() == 0) {}     //wait for data available
  String pwm_str = Serial.readString();  //read until timeout
  pwm_str.trim();                        // remove any \r \n whitespace at the end of the String
  int pwm = pwm_str.toInt();
  //TODO check conversion here
  Serial.println("Enter Desired End Pressure for airpump: ");
  while (Serial.available() == 0) {}     //wait for data available
  String target_str = Serial.readString();  //read until timeout
  target_str.trim();                        // remove any \r \n whitespace at the end of the String
  float target = target_str.toFloat();
  //TODO check conversion here
  testAirpump(pwm,target);
}


void setup() {
  Serial.begin(115200);
  pinMode(airPumpPin, OUTPUT);
  pinMode(LAC_PWM_PIN, OUTPUT);
  //fully extend actuator initially
  analogWrite(LAC_PWM_PIN, 255);  
  
  delay(1000);
  
  //inititalize pressure sensor
  if (!mpr.begin()) {
    Serial.println("Sensor init failed.");
    while (1); // Stop execution if sensor fails
  }

  //record baseline atmospheric pressure
  delay(1000);
  float pressure_hPa = mpr.readPressure();
  baseline_mmHg = pressure_hPa * HPA_TO_MMHG;

  Serial.print("Baseline Pressure Set (mmHg): ");
  Serial.println(baseline_mmHg, 3);
}

void loop() {
  //query_user_airpump_test();
  while(readRelativePressure() <= 160.0) {
    analogWrite(airPumpPin, 175);
  }
  analogWrite(airPumpPin, 0);
  Serial.println("started");
  while(1);
}
