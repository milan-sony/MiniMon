#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// OLED initialization
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Eye box parameters
const int eyeWidth = 35;
const int eyeHeight = 40;
const int eyeRadius = 10;
const int leftEyeX = 38;
const int leftEyeY = 32;
const int rightEyeX = 90;
const int rightEyeY = 32;
int pupilRadius = 10;

// Eye states
enum EyeState {
  CENTER,
  LEFT,
  RIGHT,
  BLINK
};

EyeState sequence[] = {
  CENTER, LEFT, CENTER, RIGHT, CENTER, BLINK, CENTER, BLINK, CENTER
};
const uint8_t sequenceLength = sizeof(sequence) / sizeof(sequence[0]);
uint8_t currentStep = 0;

unsigned long lastChangeTime = 0;
unsigned long interval = 800;

// Pupil positions
float currentPupilX = 0;
float currentPupilY = 0;
int8_t targetPupilX = 0;
int8_t targetPupilY = 0;

bool isBlinking = false;
bool isIdle = false;
bool isResetting = false;
unsigned long idleStartTime = 0;
unsigned long idleInterval = 2000;
unsigned long idleDurationLimit = 15000;

unsigned long resetStartTime = 0;
const unsigned long resetDuration = 1000;

unsigned long blinkStartTime = 0;
unsigned long blinkDuration = 200;

int8_t idleOffsets[][2] = {
  {0, 0}, {1, 0}, {-1, 1}, {0, -1}, {1, 1}, {-1, 0}
};
uint8_t idleIndex = 0;
uint8_t idlePatternLength = sizeof(idleOffsets) / sizeof(idleOffsets[0]);

unsigned long lastIdleBlinkTime = 0;
unsigned long idleBlinkInterval = 0;
bool idleBlinkingNow = false;

void drawEyes(float pupilOffsetX, float pupilOffsetY, bool blinking) {
  u8g2.firstPage();
  do {
    if (blinking) {
      u8g2.drawBox(leftEyeX - eyeWidth / 2, leftEyeY - 2, eyeWidth, 4);
      u8g2.drawBox(rightEyeX - eyeWidth / 2, rightEyeY - 2, eyeWidth, 4);
    } else {
      u8g2.setDrawColor(1);
      u8g2.drawRBox(leftEyeX - eyeWidth / 2, leftEyeY - eyeHeight / 2, eyeWidth, eyeHeight, eyeRadius);
      u8g2.drawRBox(rightEyeX - eyeWidth / 2, rightEyeY - eyeHeight / 2, eyeWidth, eyeHeight, eyeRadius);

      u8g2.setDrawColor(0);
      u8g2.drawDisc(leftEyeX + pupilOffsetX, leftEyeY + pupilOffsetY, pupilRadius);
      u8g2.drawDisc(rightEyeX + pupilOffsetX, rightEyeY + pupilOffsetY, pupilRadius);

      u8g2.setDrawColor(1);
      u8g2.drawDisc(leftEyeX + pupilOffsetX - 3, leftEyeY + pupilOffsetY - 3, 1);
      u8g2.drawDisc(rightEyeX + pupilOffsetX - 3, rightEyeY + pupilOffsetY - 3, 1);
    }
  } while (u8g2.nextPage());
}

void updateTarget(EyeState state) {
  switch (state) {
    case CENTER:
      targetPupilX = 0; targetPupilY = 0; isBlinking = false;
      Serial.println("CENTER");
      break;
    case LEFT:
      targetPupilX = -5; targetPupilY = 0; isBlinking = false;
      Serial.println("LEFT");
      break;
    case RIGHT:
      targetPupilX = 5; targetPupilY = 0; isBlinking = false;
      Serial.println("RIGHT");
      break;
    case BLINK:
      isBlinking = true;
      blinkDuration = random(150, 250);
      blinkStartTime = millis();
      Serial.println("BLINK");
      break;
  }
}

void randomizeSequence() {
  for (int i = 0; i < sequenceLength; i++) {
    int r = random(3);  // CENTER, LEFT, RIGHT
    sequence[i] = static_cast<EyeState>(r);
    if (random(10) < 2) sequence[i] = BLINK;
  }
}

void enterIdleMode() {
  isIdle = true;
  idleStartTime = millis();
  lastIdleBlinkTime = millis();
  idleBlinkInterval = random(3000, 7000);
}

void exitIdleMode() {
  isResetting = true;
  resetStartTime = millis();
  targetPupilX = 0;
  targetPupilY = 0;
  isBlinking = true;
  blinkDuration = random(150, 250);
  blinkStartTime = millis();
  Serial.println("RESET TO CENTER");
}

void handleResetTransition() {
  unsigned long now = millis();

  if (isBlinking && now - blinkStartTime > blinkDuration / 2) {
    isBlinking = false;
  }

  if (now - resetStartTime >= resetDuration) {
    isResetting = false;
    isIdle = false;
    idleBlinkingNow = false;
    currentStep = 0;
    randomizeSequence();
    updateTarget(sequence[currentStep]);
    lastChangeTime = now;
  }
}

void updateIdleMovement() {
  unsigned long now = millis();

  float shimmerX = cos(now / 700.0) * 0.5;
  float shimmerY = sin(now / 500.0) * 0.5;

  targetPupilX = idleOffsets[idleIndex][0] + shimmerX;
  targetPupilY = idleOffsets[idleIndex][1] + shimmerY;

  if (now - idleStartTime > idleInterval) {
    idleIndex = (idleIndex + 1) % idlePatternLength;
    idleStartTime = now;
    idleInterval = random(1000, 2500);
  }

  if (!idleBlinkingNow && (now - lastIdleBlinkTime >= idleBlinkInterval)) {
    idleBlinkingNow = true;
    isBlinking = true;
    blinkStartTime = now;
    lastIdleBlinkTime = now;
    blinkDuration = random(150, 250);
    idleBlinkInterval = random(3000, 7000);
    Serial.println("Idle Blink");
  }

  if (idleBlinkingNow && (now - blinkStartTime > blinkDuration)) {
    idleBlinkingNow = false;
    isBlinking = false;
  }

  if (!isResetting && isIdle && now - lastChangeTime > idleDurationLimit) {
    exitIdleMode();
  }
}

void smoothMovePupil() {
  float ease = random(18, 22) / 100.0;
  currentPupilX += (targetPupilX - currentPupilX) * ease;
  currentPupilY += (targetPupilY - currentPupilY) * ease;

  if (abs(targetPupilX - currentPupilX) < 0.1) currentPupilX = targetPupilX;
  if (abs(targetPupilY - currentPupilY) < 0.1) currentPupilY = targetPupilY;
}

void setup() {
  u8g2.begin();
  Serial.begin(9600);
  randomSeed(analogRead(0));
  updateTarget(sequence[currentStep]);
  lastChangeTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (isResetting) {
    handleResetTransition();
  } else if (!isIdle) {
    EyeState currentState = sequence[currentStep];
    interval = (currentState == BLINK) ? 150 : random(800, 1400);

    if (currentTime - lastChangeTime >= interval) {
      currentStep++;
      if (currentStep >= sequenceLength) {
        isBlinking = true;
        blinkDuration = random(150, 250);
        blinkStartTime = millis();
        delay(blinkDuration);
        isBlinking = false;
        enterIdleMode();
      } else {
        updateTarget(sequence[currentStep]);
        lastChangeTime = currentTime;
      }
    }
  } else {
    updateIdleMovement();
  }

  smoothMovePupil();
  drawEyes(currentPupilX, currentPupilY, isBlinking);
  delay(25);
}
