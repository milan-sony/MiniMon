#include <Servo.h>

int servoPin = 9;
Servo servo1;

void setup() {
  servo1.attach(servoPin);
  Serial.begin(9600);
}

void loop() {
  // Move to Left
  servo1.write(0);
  Serial.println("Servo Arm Set to LEFT (0°)");
  delay(2000);

  // Move to Straight
  servo1.write(90);
  Serial.println("Servo Arm Set to STRAIGHT (90°)");
  delay(2000);

  // Move to Right
  servo1.write(180);
  Serial.println("Servo Arm Set to RIGHT (180°)");
  delay(2000);

  // Back to Straight
  servo1.write(90);
  Serial.println("Servo Arm Returned to STRAIGHT (90°)");
  delay(2000);
}
