#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C

#define TOUCH_PIN 2
#define WATER_SENSOR A1
#define SOIL_SENSOR  A0

#define PUMP_IN1 6
#define PUMP_IN2 7
#define PUMP_EN  10

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SH1106G> roboEyes(display);

int lastTouchState = LOW;
unsigned long touchStart = 0;
bool isHappy = false;

unsigned long lastInteraction = 0;
const unsigned long SLEEP_TIMEOUT = 300000;
bool isSleeping = false;
bool longPressHandled = false;

unsigned long pumpTimer = 0;
bool pumpRunning = false;
const unsigned long PUMP_INTERVAL = 5000;
const unsigned long PUMP_DURATION = 3000;

unsigned long sensorTimer = 0;
int waterValue = 0;
bool isAngry = false;
bool infoMode = false; // true ise info ekranı açık

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(TOUCH_PIN, INPUT);
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(PUMP_EN, OUTPUT);

  digitalWrite(PUMP_IN1, LOW);
  digitalWrite(PUMP_IN2, LOW);
  digitalWrite(PUMP_EN, LOW);

  display.begin(I2C_ADDRESS, true);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  showLoadingScreen();

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  roboEyes.setBorderradius(8, 8);
  roboEyes.setSpacebetween(10);
  roboEyes.setMood(DEFAULT);

  lastInteraction = millis();
  pumpTimer = millis();
  sensorTimer = millis();
}

void loop() {
  unsigned long now = millis();
  int touchState = digitalRead(TOUCH_PIN);

  // ---------- Sensör ----------
  if (now - sensorTimer >= 1000) {
    sensorTimer = now;
    waterValue = analogRead(WATER_SENSOR);
    int soilValue = analogRead(SOIL_SENSOR);

    Serial.print(waterValue);
    Serial.print(",");
    Serial.println(soilValue);
  }

// ---------- Suya göre yüz ----------
if (!isHappy && !isSleeping && !isAngry) {
  if (waterValue < 650) {
    roboEyes.setMood(TIRED);
  } else {
    roboEyes.setMood(DEFAULT);
  }
}

// parmağı çekince

// ---------- Dokunma ----------
if (touchState == HIGH && lastTouchState == LOW) {
  lastInteraction = now;
  touchStart = now;
  isHappy = true;
  isAngry = false;
  longPressHandled = false;

  roboEyes.setMood(HAPPY);
  roboEyes.anim_laugh();
}

// 3 saniye basılı tutulursa KIZ
if (touchState == HIGH && isHappy && !longPressHandled && (now - touchStart >= 3000)) {
  roboEyes.setMood(ANGRY);
  isAngry = true;
  longPressHandled = true;
  isHappy = false;
}

// Parmağı çekince reset
if (touchState == LOW && lastTouchState == HIGH) {
  isHappy = false;
  isAngry = false;
  longPressHandled = false;
}



  // ---------- Uyku ----------
  if (!isSleeping && (now - lastInteraction >= SLEEP_TIMEOUT)) {
    isSleeping = true;
    roboEyes.close();
  }

  if (!isSleeping) {
    roboEyes.update();
  }

  // ---------- Pompa ----------
  if (!pumpRunning && (now - pumpTimer >= PUMP_INTERVAL)) {
    pumpRunning = true;
    pumpTimer = now;
    digitalWrite(PUMP_IN1, HIGH);
    digitalWrite(PUMP_IN2, LOW);
    digitalWrite(PUMP_EN, HIGH);
  }

  if (pumpRunning && (now - pumpTimer >= PUMP_DURATION)) {
    pumpRunning = false;
    pumpTimer = now;
    digitalWrite(PUMP_IN1, LOW);
    digitalWrite(PUMP_IN2, LOW);
    digitalWrite(PUMP_EN, LOW);
  }

  lastTouchState = touchState;
}

void showLoadingScreen() {
  int barWidth = SCREEN_WIDTH - 20;
  int barHeight = 10;
  int x = 10;
  int y = (SCREEN_HEIGHT - barHeight) / 2;

  for (int i = 0; i <= 100; i++) {
    display.clearDisplay();
    display.setCursor(10, 10);
    display.println("TEPELI INDUSTRIES");
    display.setCursor(0, SCREEN_HEIGHT - 20);
    display.println("Akilli Bitki Saksisi MARK 4.1");
    display.drawRect(x, y, barWidth, barHeight, SH110X_WHITE);
    display.fillRect(x, y, (barWidth * i) / 100, barHeight, SH110X_WHITE);
    display.display();
    delay(60);
  }

  display.clearDisplay();
  display.display();
}
