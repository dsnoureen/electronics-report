#include <ESP32Servo.h>

// === Servo Pins ===
const int SERVO1_PIN = 19;
const int SERVO2_PIN = 18;
const int SERVO3_PIN = 23;

// === IR Sensor Pins (Inputs) ===
const int IR1 = 36;
const int IR2 = 33;
const int IR3 = 39;
const int IR4 = 35;
const int IR5 = 34;
const int IR6 = 32;

// === LED Pin Assignments ===
const int LED1 = 13;
const int LED2 = 12;
const int LED3 = 27;
const int LED4 = 14;
const int LED5 = 26;
const int LED6 = 25;

// === Servo objects ===
Servo servo1;
Servo servo2;
Servo servo3;

// === LED control struct ===
struct LEDBlink {
  int pin;
  bool active;
  unsigned long previousMillis;
  bool state;
};

LEDBlink leds[6] = {
  { LED1, false, 0, false },
  { LED2, false, 0, false },
  { LED3, false, 0, false },
  { LED4, false, 0, false },
  { LED5, false, 0, false },
  { LED6, false, 0, false }
};

const unsigned long blinkInterval = 300;  // ms between on/off

void setup() {
  Serial.begin(115200);
  Serial.println("Servo + IR + LED Command Controller Ready");

  // Attach servos
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);

  servo1.write(90);
  servo2.write(40);
  servo3.write(60);

  // Configure IR pins
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  pinMode(IR4, INPUT);
  pinMode(IR5, INPUT);
  pinMode(IR6, INPUT);

  // Configure LED pins
  for (int i = 0; i < 6; i++) {
    pinMode(leds[i].pin, OUTPUT);
    digitalWrite(leds[i].pin, LOW);
  }
}

void loop() {
  updateLEDs();

  if (Serial.available()) {

    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "m1") {
      servo1.write(170);  // Pivot to 170
      delay(20);
      servo2.write(40);   // Lid 1 Close
      delay(20);
      servo3.write(180);  // Lid 2 Open
      delay(20);
      activateBlink(0);
      Serial.println("m1: servo1=170, servo2=40, servo3=180");
    }

    if (input == "m2") {
      servo1.write(170);  // Pivot to 170
      delay(20);
      servo2.write(40);  // Lid 1 Close
      delay(20);
      servo3.write(180);  // Lid 2 Open
      delay(20);
      activateBlink(1);
      Serial.println("m2: servo1=170, servo2=40, servo3=180");
    }

    if (input == "e1") {
      servo1.write(100);  // Pivot to 100
      delay(20);
      servo2.write(160);  // Lid 1 Open
      delay(20);
      servo3.write(60);  // Lid 2 Close
      delay(20);
      activateBlink(2);
      Serial.println("e1: servo1=100, servo2=160");
    }

    if (input == "e2") {
      servo1.write(100);  // Pivot to 100
      delay(20);
      servo2.write(160);  // Lid 1 Open
      delay(20);
      servo3.write(60);  // Lid 2 Close
      delay(20);
      activateBlink(3);
      Serial.println("e2: servo1=100, servo2=160");
    }

    if (input == "n1") {
      servo1.write(50);  // Pivot to 50
      delay(20);
      servo2.write(40);  // Lid 1 Close
      delay(20);
      servo3.write(180);  //
      delay(20);
      activateBlink(4);
      Serial.println("n1: servo1=50, servo2=180");
    }

    if (input == "n2") {
      servo1.write(50);  // Pivot to 50
      delay(20);
      servo2.write(40);  // Lid 1 Close
      delay(20);
      servo3.write(180);  // Lid 2 Open
      delay(20);
      activateBlink(5);
      Serial.println("n2: servo1=50, servo2=180");
    }

    if (input == "closeall") {
      servo2.write(40);
      delay(20);
      servo3.write(60);
      delay(20);
      stopAllBlinks();
      Serial.println("closeall: servo2=40, servo3=60 (LEDs stopped)");
    }

    if (input == "irarray") {
      int irValues[6] = {
        digitalRead(IR1),
        digitalRead(IR2),
        digitalRead(IR3),
        digitalRead(IR4),
        digitalRead(IR5),
        digitalRead(IR6)
      };

      for (int i = 0; i < 6; i++) {
        Serial.print(irValues[i]);
        if (i < 5) Serial.print(",");
      }
      Serial.println();
    }
  }
}

// === LED control functions ===
void activateBlink(int index) {
  leds[index].active = true;
  leds[index].previousMillis = millis();
}

void stopAllBlinks() {
  for (int i = 0; i < 6; i++) {
    leds[i].active = false;
    digitalWrite(leds[i].pin, LOW);
    leds[i].state = false;
  }
}

void updateLEDs() {
  unsigned long currentMillis = millis();
  for (int i = 0; i < 6; i++) {
    if (leds[i].active && currentMillis - leds[i].previousMillis >= blinkInterval) {
      leds[i].previousMillis = currentMillis;
      leds[i].state = !leds[i].state;
      digitalWrite(leds[i].pin, leds[i].state);
    }
  }
}
