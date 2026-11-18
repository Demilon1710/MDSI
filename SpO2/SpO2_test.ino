const int SAMPLE_DELAY = 40;
float TARGET_BPM = 60.0; 

int IR_READ_PIN = A4;
int R_READ_PIN = A3;

int IR_TRANS_PIN = 3;
int R_TRANS_PIN = 5;

int IR_READ_VAL = 0;
int R_READ_VAL = 0;
int IR_WRITE_VAL = 0;
int R_WRITE_VAL = 0;

float IR_sineVal;
float R_sineVal;

float OSC_MAG = 5.0;

void setup() {
  Serial.begin(9600);
  Serial.println("Good morning!");

  pinMode(R_TRANS_PIN, OUTPUT);
  pinMode(IR_TRANS_PIN, OUTPUT);
}

float getSin(float BPM, float phi){
  float t = millis()/1000.0; 
  return float(sin(2.0*PI*t*(BPM/60.0) + phi));

}
void loop() {

  R_READ_VAL = analogRead(R_READ_PIN);
  IR_READ_VAL = analogRead(IR_READ_PIN);

  IR_sineVal = (float) IR_READ_VAL + OSC_MAG * getSin(TARGET_BPM, 0.0*PI);
  R_sineVal = (float) R_READ_VAL + OSC_MAG * getSin(TARGET_BPM, 0.0);

  analogWrite(IR_TRANS_PIN, IR_sineVal + 100.00);
  analogWrite(R_TRANS_PIN, R_sineVal + 100.00);

  Serial.print(IR_READ_VAL);
  Serial.print(", ");
  Serial.print(R_READ_VAL);
  Serial.print(", ");
  Serial.print(IR_sineVal);
  Serial.print(", ");
  Serial.println(R_sineVal);

  delay(SAMPLE_DELAY);

}